#include "ng/engine/opengl/glrenderer.hpp"

#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/window.hpp"
#include "ng/engine/window/glcontext.hpp"

#include "ng/engine/rendering/renderbatch.hpp"

#include "ng/engine/util/scopeguard.hpp"

namespace ng
{

static void RenderingThread(RenderingThreadData& threadData);

class OpenGLRenderBatch : public IRenderBatch, public std::enable_shared_from_this<OpenGLRenderBatch>
{
    std::shared_ptr<OpenGLRenderer> mRenderer;

    bool mShouldSwapBuffers = false;
    bool mShouldSwapBuffersShadow = false;

public:
    friend void RenderingThread(RenderingThreadData&);

    OpenGLRenderBatch(std::shared_ptr<OpenGLRenderer> renderer)
        : mRenderer(std::move(renderer))
    { }

    void Commit() override
    {
        // become sole producer for the commit
        mRenderer->mRenderingThreadData.BatchProducerLock.lock();

        auto productionScope = make_scope_guard([&]{
            // give the produced batch to the renderer
            mRenderer->mRenderingThreadData.BatchConsumerLock.unlock();
        });

        // commit changes to the batch
        mShouldSwapBuffers = mShouldSwapBuffersShadow;

        mRenderer->mRenderingThreadData.CurrentBatch = shared_from_this();
    }

    void SetShouldSwapBuffers(bool shouldSwapBuffers) override
    {
        mShouldSwapBuffersShadow = shouldSwapBuffers;
    }
};

std::shared_ptr<IRenderBatch> OpenGLRenderer::CreateRenderBatch()
{
    return std::make_shared<OpenGLRenderBatch>(shared_from_this());
}

RenderingThreadData::RenderingThreadData(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window)
    : WindowManager(std::move(windowManager))
    , Window(std::move(window))
    , ShouldQuit(false)
{
    BatchConsumerLock.lock();
}

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
        std::shared_ptr<const OpenGLRenderBatch> batchPtr = threadData.CurrentBatch;
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

        const OpenGLRenderBatch& batch = *batchPtr;

        if (batch.mShouldSwapBuffers)
        {
            threadData.Window->SwapBuffers();
        }
    }
}

OpenGLRenderer::OpenGLRenderer(std::shared_ptr<IWindowManager> windowManager,
                               std::shared_ptr<IWindow> window)
    : mRenderingThreadData(windowManager, window)
    , mRenderingThread(RenderingThread, std::ref(mRenderingThreadData))
{ }

OpenGLRenderer::~OpenGLRenderer()
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

} // end namespace ng
