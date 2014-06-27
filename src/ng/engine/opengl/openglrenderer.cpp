#include "ng/engine/opengl/openglrenderer.hpp"

#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/window.hpp"
#include "ng/engine/window/glcontext.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/renderbatch.hpp"

#include "ng/engine/util/scopeguard.hpp"

#include <thread>
#include <mutex>

namespace ng
{

class RenderingThreadData
{
public:
    const std::shared_ptr<IWindowManager> WindowManager;
    const std::shared_ptr<IWindow> Window;

    std::mutex BatchConsumerLock;
    std::mutex BatchProducerLock;

    bool ShouldQuit;

    std::shared_ptr<const IRenderBatch> CurrentBatch;

    RenderingThreadData(std::shared_ptr<IWindowManager> windowManager,
                        std::shared_ptr<IWindow> window)
        : WindowManager(std::move(windowManager))
        , Window(std::move(window))
        , ShouldQuit(false)
    {
        BatchConsumerLock.lock();
    }
};

class OpenGLRendererBase : public IRenderer
{
public:
    virtual RenderingThreadData& GetRenderingThreadData() = 0;
    virtual const RenderingThreadData& GetRenderingThreadData() const = 0;
};

static void RenderingThread(RenderingThreadData& threadData);

class OpenGLRenderBatch : public IRenderBatch, public std::enable_shared_from_this<OpenGLRenderBatch>
{
    std::shared_ptr<OpenGLRendererBase> mRenderer;

    bool mShouldSwapBuffers = false;
    bool mShouldSwapBuffersShadow = false;

public:
    OpenGLRenderBatch(std::shared_ptr<OpenGLRendererBase> renderer)
        : mRenderer(std::move(renderer))
    { }

    void Commit() override
    {
        // become sole producer for the commit
        mRenderer->GetRenderingThreadData().BatchProducerLock.lock();

        auto productionScope = make_scope_guard([&]{
            // give the produced batch to the renderer
            mRenderer->GetRenderingThreadData().BatchConsumerLock.unlock();
        });

        // commit changes to the batch
        mShouldSwapBuffers = mShouldSwapBuffersShadow;

        mRenderer->GetRenderingThreadData().CurrentBatch = shared_from_this();
    }

    void SetShouldSwapBuffers(bool shouldSwapBuffers) override
    {
        mShouldSwapBuffersShadow = shouldSwapBuffers;
    }

    bool GetShouldSwapBuffers() const override
    {
        return mShouldSwapBuffersShadow;
    }

    bool GetShouldSwapBuffersBack() const
    {
        return mShouldSwapBuffers;
    }

    MeshInstanceID AddMeshInstance(MeshID meshID) override
    {
        throw std::runtime_error("OpenGLRenderBatch::AddMeshInstance not implemented");
    }

    void RemoveMeshInstance(MeshInstanceID meshInstanceID) override
    {
        throw std::runtime_error("GLRenderBatch::RemoveMeshInstance");
    }
};

class OpenGLRenderer : public OpenGLRendererBase, public std::enable_shared_from_this<OpenGLRenderer>
{
    RenderingThreadData mRenderingThreadData;
    std::thread mRenderingThread;

public:
          RenderingThreadData& GetRenderingThreadData() override { return mRenderingThreadData; }
    const RenderingThreadData& GetRenderingThreadData() const override { return mRenderingThreadData; }

    OpenGLRenderer(std::shared_ptr<IWindowManager> windowManager,
                   std::shared_ptr<IWindow> window)
        : mRenderingThreadData(std::move(windowManager), std::move(window))
        , mRenderingThread(RenderingThread, std::ref(mRenderingThreadData))
    { }

    ~OpenGLRenderer()
    {
        // wait for the current batch to finish processing.
        mRenderingThreadData.BatchProducerLock.lock();

        // signal the end of rendering.
        mRenderingThreadData.ShouldQuit = true;

        // allow the renderer to quit.
        mRenderingThreadData.BatchConsumerLock.unlock();

        // join the now quit rendering thread.
        mRenderingThread.join();
    }

    std::shared_ptr<IRenderBatch> CreateRenderBatch() override
    {
        return std::make_shared<OpenGLRenderBatch>(shared_from_this());
    }

    MeshID AddMesh(std::unique_ptr<IMesh> mesh) override
    {
        throw std::runtime_error("OpenGLRenderer::AddMesh not yet implemented");
    }

    void RemoveMesh(MeshID meshID) override
    {
        throw std::runtime_error("OpenGLRenderer::RemoveMesh not yet implemented");
    }
};

void RenderingThread(RenderingThreadData& threadData)
{
    std::shared_ptr<IGLContext> context = threadData.WindowManager->CreateGLContext(
                threadData.Window->GetVideoFlags(), nullptr);
    threadData.WindowManager->SetCurrentGLContext(threadData.Window, context);

    while (true)
    {
        // wait for a batch to be available to consume.
        threadData.BatchConsumerLock.lock();

        auto consumptionScope = make_scope_guard([&]{
            // allow the producer to submit another batch again.
            threadData.BatchProducerLock.unlock();
        });

        // grab the batch to render it
        std::shared_ptr<const OpenGLRenderBatch> batchPtr = std::static_pointer_cast<const OpenGLRenderBatch>(threadData.CurrentBatch);
        threadData.CurrentBatch = nullptr;

        // check if the renderer requested termination
        if (threadData.ShouldQuit)
        {
            break;
        }

        // was given nothing to do?
        if (batchPtr == nullptr)
        {
            continue;
        }

        // do the work requested in the batch
        const OpenGLRenderBatch& batch = *batchPtr;

        if (batch.GetShouldSwapBuffersBack())
        {
            threadData.Window->SwapBuffers();
        }
    }
}

std::shared_ptr<IRenderer> CreateOpenGLRenderer(std::shared_ptr<IWindowManager> windowManager,
                                                std::shared_ptr<IWindow> window)
{
    return std::make_shared<OpenGLRenderer>(std::move(windowManager), std::move(window));
}

} // end namespace ng
