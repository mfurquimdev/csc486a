#include "ng/engine/renderer.hpp"

#include "ng/engine/windowmanager.hpp"
#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"

#include <thread>

namespace ng
{

static void RenderingThreadEntry()
{

}

static void ResourceThreadEntry()
{

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
            const std::shared_ptr<IWindow>& window,
            const std::shared_ptr<IGLContext>& renderingContext,
            const std::shared_ptr<IGLContext>& resourceContext)
        : mWindow(window)
        , mRenderingContext(renderingContext)
        , mResourceContext(resourceContext)
        , mRenderingThread(RenderingThreadEntry)
        , mResourceThread(ResourceThreadEntry)
    {

    }

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
    std::shared_ptr<IGLContext> renderingContext = windowManager->CreateContext(window->GetVideoFlags(), nullptr);
    std::shared_ptr<IGLContext> resourceContext = windowManager->CreateContext(window->GetVideoFlags(), renderingContext);
    return std::shared_ptr<GL3Renderer>(new GL3Renderer(window, renderingContext, resourceContext));
}

} // end namespace ng
