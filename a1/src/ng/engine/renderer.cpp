#include "ng/engine/renderer.hpp"

#include "ng/engine/windowmanager.hpp"
#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"

#include <thread>

namespace ng
{

struct RenderingThreadData
{
    std::shared_ptr<IWindowManager> mWindowManager;
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mContext;
};

struct ResourceThreadData
{
    std::shared_ptr<IWindowManager> mWindowManager;
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mContext;
};

static void RenderingThreadEntry(RenderingThreadData threadData)
{
    threadData.mWindowManager->SetCurrentContext(threadData.mWindow, threadData.mContext);
}

static void ResourceThreadEntry(ResourceThreadData threadData)
{
    threadData.mWindowManager->SetCurrentContext(threadData.mWindow, threadData.mContext);
}

class GL3Renderer: public IRenderer
{
public:
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mRenderingContext;
    std::shared_ptr<IGLContext> mResourceContext;

    std::thread mRenderingThread;
    std::thread mResourceThread;

    GL3Renderer(
            const std::shared_ptr<IWindowManager>& windowManager,
            const std::shared_ptr<IWindow>& window)
        : mWindow(window)
        , mRenderingContext(windowManager->CreateContext(window->GetVideoFlags(), nullptr))
        , mResourceContext(windowManager->CreateContext(window->GetVideoFlags(), mRenderingContext))
        , mRenderingThread(RenderingThreadEntry, RenderingThreadData { windowManager, window, mRenderingContext })
        , mResourceThread(ResourceThreadEntry, ResourceThreadData { windowManager, window, mResourceContext })
    { }

    ~GL3Renderer()
    {
        // TODO: Must give the threads a signal to tell them that they should start shutting down.
        mResourceThread.join();
        mRenderingThread.join();
    }
};

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window)
{
    return std::shared_ptr<GL3Renderer>(new GL3Renderer(windowManager, window));
}

} // end namespace ng
