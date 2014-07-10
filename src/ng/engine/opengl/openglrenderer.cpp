#include "ng/engine/opengl/openglrenderer.hpp"

#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/window.hpp"
#include "ng/engine/window/glcontext.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/scenegraph.hpp"

#include "ng/engine/opengl/opengles2commandvisitor.hpp"
#include "ng/engine/opengl/openglcommands.hpp"

#include "ng/engine/util/scopeguard.hpp"
#include "ng/engine/util/memory.hpp"

#include "ng/engine/util/debug.hpp"

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>

namespace ng
{

class RenderingThreadData
{
public:
    const std::shared_ptr<IWindowManager> WindowManager;
    const std::shared_ptr<IWindow> Window;

    using CommandVisitorFactoryType =
        std::function<
            std::shared_ptr<IRendererCommandVisitor>(
                IGLContext& context, IWindow& window)>;

    using CommandQueueType =
        std::vector<
            std::unique_ptr<IRendererCommand>>;

    CommandVisitorFactoryType VisitorFactory;
    std::shared_ptr<IRendererCommandVisitor> Visitor;

    std::mutex BatchConsumerLock;
    std::mutex BatchProducerLock;

    std::mutex RendererReady;

    std::atomic<bool> RendererDied{false};

    CommandQueueType CommandQueue;

    RenderingThreadData(std::shared_ptr<IWindowManager> windowManager,
                        std::shared_ptr<IWindow> window)
        : WindowManager(std::move(windowManager))
        , Window(std::move(window))
    {
        // make data initially unavailable to the consumer
        BatchConsumerLock.lock();

        // renderer is initially not ready
        RendererReady.lock();
    }
};

class OpenGLRenderer : public IRenderer, public std::enable_shared_from_this<OpenGLRenderer>
{
    RenderingThreadData mRenderingThreadData;

    bool mUseRenderingThread;
    std::thread mRenderingThread;

    enum class RendererState
    {
        InsideFrame,
        OutsideFrame
    };

    RendererState mState = RendererState::OutsideFrame;
    std::mutex mInterfaceMutex;

    static void SetupGLContextAndVisitor(RenderingThreadData& threadData)
    {
        std::shared_ptr<IGLContext> context =
                threadData.WindowManager->CreateGLContext(
                    threadData.Window->GetVideoFlags(), nullptr);

        threadData.WindowManager->SetCurrentGLContext(threadData.Window, context);

        threadData.Visitor =
                threadData.VisitorFactory(
                    *context,
                    *threadData.Window);
    }

    static void RenderingThread(RenderingThreadData& threadData) try
    {
        auto setFlagOnDeath = make_scope_guard([&]{
            threadData.RendererDied = true;
        });

        RenderingThreadData::CommandQueueType commandQueue;

        {
            auto readyScope = make_scope_guard([&]{
               threadData.RendererReady.unlock();
            });

            SetupGLContextAndVisitor(threadData);
        }

        bool shouldQuit = false;

        while (!shouldQuit) try
        {
            // grab a batch of commands
            {
                // wait for a batch to be available to consume.
                threadData.BatchConsumerLock.lock();

                auto consumptionScope = make_scope_guard([&]{
                    // allow the producer to submit another batch again.
                    threadData.BatchProducerLock.unlock();
                });

                // grab the data
                commandQueue.swap(threadData.CommandQueue);
            }

            auto commandClearScope = make_scope_guard([&]{
                commandQueue.clear();
            });

            if (threadData.Visitor == nullptr)
            {
                continue;
            }

            IRendererCommandVisitor& commandVisitor = *threadData.Visitor;

            auto quitScope = make_scope_guard([&]{
                shouldQuit = commandVisitor.ShouldQuit();
            });

            for (std::unique_ptr<IRendererCommand>& cmd : commandQueue)
            {
                if (cmd != nullptr)
                {
                    cmd->Accept(commandVisitor);
                }
            }
        }
        catch (const std::exception& e)
        {
            ng::DebugPrintf("RenderingThread loop error: %s\n", e.what());
        }
        catch (...)
        {
            ng::DebugPrintf("RenderingThread loop error: (unknown)\n");
        }
    }
    catch (const std::exception& e)
    {
        ng::DebugPrintf("RenderingThread error: %s\n", e.what());
    }
    catch (...)
    {
        ng::DebugPrintf("RenderingThread error: (unknown)\n");
    }

public:
    OpenGLRenderer(std::shared_ptr<IWindowManager> windowManager,
                   std::shared_ptr<IWindow> window,
                   bool useRenderingThread)
        : mRenderingThreadData(
              std::move(windowManager),
              std::move(window))
        , mUseRenderingThread(useRenderingThread)
    {
        mRenderingThreadData.VisitorFactory =
                [](IGLContext& context, IWindow& window)
        {
            return std::make_shared<OpenGLES2CommandVisitor>(
                        context, window);
        };

        if (mUseRenderingThread)
        {
            mRenderingThread =
                    std::thread(
                        RenderingThread,
                        std::ref(mRenderingThreadData));

            // wait until the renderer is ready
            mRenderingThreadData.RendererReady.lock();

            if (mRenderingThreadData.RendererDied)
            {
                mRenderingThread.join();
                throw std::runtime_error("Failed to initialize rendering thread.");
            }
        }
        else
        {
            SetupGLContextAndVisitor(mRenderingThreadData);
        }
    }

    ~OpenGLRenderer()
    {
        if (mUseRenderingThread)
        {
            // wait for the current batch to finish processing.
            mRenderingThreadData.BatchProducerLock.lock();

            // signal the end of rendering.
            mRenderingThreadData.CommandQueue.push_back(
                        ng::make_unique<QuitCommand>());

            // allow the renderer to quit.
            mRenderingThreadData.BatchConsumerLock.unlock();

            // join the now quit rendering thread.
            mRenderingThread.join();
        }
    }

    void BeginFrame(vec3 clearColor) override
    {
        std::unique_lock<std::mutex> interfaceLock(
                    mInterfaceMutex,
                    std::defer_lock);

        if (mUseRenderingThread)
        {
            interfaceLock.lock();
        }

        if (mState != RendererState::OutsideFrame)
        {
            throw std::logic_error("BeginFrame() called when already in a frame");
        }

        mState = RendererState::InsideFrame;

        if (mUseRenderingThread)
        {
            // make sure the rendering thread is done consuming the queued up commands
            mRenderingThreadData.BatchProducerLock.lock();
        }

        mRenderingThreadData.CommandQueue.push_back(
                    ng::make_unique<BeginFrameCommand>(clearColor));
    }

    void Render(const SceneGraph& scene) override
    {
        std::unique_lock<std::mutex> interfaceLock(
                    mInterfaceMutex,
                    std::defer_lock);

        if (mUseRenderingThread)
        {
            interfaceLock.lock();
        }

        if (mState != RendererState::InsideFrame)
        {
            throw std::logic_error("Render() called when not in a frame");
        }

        mRenderingThreadData.CommandQueue.push_back(
                    ng::make_unique<RenderBatchCommand>(
                        RenderBatch::FromScene(scene)));
    }

    void EndFrame() override
    {
        std::unique_lock<std::mutex> interfaceLock(
                    mInterfaceMutex,
                    std::defer_lock);

        if (mUseRenderingThread)
        {
            interfaceLock.lock();
        }

        if (mState != RendererState::InsideFrame)
        {
            throw std::logic_error("EndFrame() called when not in a frame");
        }

        mState = RendererState::OutsideFrame;

        mRenderingThreadData.CommandQueue.push_back(
                    ng::make_unique<EndFrameCommand>());

        if (mUseRenderingThread)
        {
            // let the renderer eat up the data
            mRenderingThreadData.BatchConsumerLock.unlock();
        }
        else
        {
            auto clearCommandScope = make_scope_guard([&]{
                mRenderingThreadData.CommandQueue.clear();
            });

            if (mRenderingThreadData.Visitor != nullptr)
            {
                IRendererCommandVisitor& visitor =
                        *mRenderingThreadData.Visitor;

                for (std::unique_ptr<IRendererCommand>& cmd
                     : mRenderingThreadData.CommandQueue)
                {
                    if (cmd != nullptr)
                    {
                        cmd->Accept(visitor);
                    }
                }
            }
        }
    }
};

std::shared_ptr<IRenderer> CreateOpenGLRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window,
        bool useRenderingThread)
{
    return std::make_shared<OpenGLRenderer>(
                std::move(windowManager),
                std::move(window),
                useRenderingThread);
}

} // end namespace ng
