#include "ng/engine/opengl/openglrenderer.hpp"

#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/window.hpp"
#include "ng/engine/window/glcontext.hpp"

#include "ng/engine/rendering/renderer.hpp"

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

    std::mutex BatchConsumerLock;
    std::mutex BatchProducerLock;

    std::vector<std::unique_ptr<IRendererCommand>> CommandQueue;

    RenderingThreadData(std::shared_ptr<IWindowManager> windowManager,
                        std::shared_ptr<IWindow> window)
        : WindowManager(std::move(windowManager))
        , Window(std::move(window))
    {
        // make data initially unavailable to the consumer
        BatchConsumerLock.lock();
    }
};

class OpenGLRenderer : public IRenderer, public std::enable_shared_from_this<OpenGLRenderer>
{
    RenderingThreadData mRenderingThreadData;
    std::thread mRenderingThread;

    enum class RendererState
    {
        InsideFrame,
        OutsideFrame
    };

    RendererState mState = RendererState::OutsideFrame;
    std::mutex mInterfaceMutex;

    static void RenderingThread(RenderingThreadData& threadData)
    {
        std::shared_ptr<IGLContext> context = threadData.WindowManager->CreateGLContext(
                    threadData.Window->GetVideoFlags(), nullptr);
        threadData.WindowManager->SetCurrentGLContext(threadData.Window, context);

        std::unique_ptr<IRendererCommandVisitor> pCommandVisitor =
                ng::make_unique<OpenGLES2CommandVisitor>(
                    context, threadData.Window);

        std::vector<std::unique_ptr<IRendererCommand>> commands;

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
                commands.swap(threadData.CommandQueue);
            }

            auto commandClearScope = make_scope_guard([&]{
                commands.clear();
            });

            if (pCommandVisitor == nullptr)
            {
                continue;
            }

            IRendererCommandVisitor& commandVisitor = *pCommandVisitor;

            auto quitScope = make_scope_guard([&]{
                shouldQuit = commandVisitor.ShouldQuit();
            });

            for (std::unique_ptr<IRendererCommand>& cmd : commands)
            {
                if (cmd != nullptr)
                {
                    cmd->Accept(commandVisitor);
                }
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
    }

public:
    OpenGLRenderer(std::shared_ptr<IWindowManager> windowManager,
                   std::shared_ptr<IWindow> window)
        : mRenderingThreadData(
              std::move(windowManager),
              std::move(window))
        , mRenderingThread(
              RenderingThread,
              std::ref(mRenderingThreadData))
    { }

    ~OpenGLRenderer()
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

    void BeginFrame() override
    {
        std::lock_guard<std::mutex> interfaceLock(mInterfaceMutex);

        if (mState != RendererState::OutsideFrame)
        {
            throw std::logic_error("BeginFrame() called when already in a frame");
        }

        mState = RendererState::InsideFrame;

        // make sure the rendering thread is done consuming the queued up commands
        mRenderingThreadData.BatchProducerLock.lock();

        mRenderingThreadData.CommandQueue.push_back(
                    ng::make_unique<BeginFrameCommand>());
    }

    void Render(
            std::unique_ptr<const RenderObject[]> renderObjects,
            std::size_t numRenderObjects)
    {
        std::lock_guard<std::mutex> interfaceLock(mInterfaceMutex);

        if (mState != RendererState::InsideFrame)
        {
            throw std::logic_error("Render() called when not in a frame");
        }

        mRenderingThreadData.CommandQueue.push_back(
                    ng::make_unique<RenderObjectsCommand>(
                        std::move(renderObjects),
                        numRenderObjects));
    }

    void EndFrame() override
    {
        std::lock_guard<std::mutex> interfaceLock(mInterfaceMutex);

        if (mState != RendererState::InsideFrame)
        {
            throw std::logic_error("EndFrame() called when not in a frame");
        }

        mState = RendererState::OutsideFrame;

        mRenderingThreadData.CommandQueue.push_back(
                    ng::make_unique<EndFrameCommand>());

        // let the renderer eat up the data
        mRenderingThreadData.BatchConsumerLock.unlock();
    }
};

std::shared_ptr<IRenderer> CreateOpenGLRenderer(std::shared_ptr<IWindowManager> windowManager,
                                                std::shared_ptr<IWindow> window)
{
    return std::make_shared<OpenGLRenderer>(std::move(windowManager), std::move(window));
}

} // end namespace ng
