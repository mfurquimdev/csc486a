#include "ng/engine/emscripten/emscriptenwindow.hpp"

#include "ng/engine/window/windowmanager.hpp"

#include "ng/engine/egl/eglwindow.hpp"

namespace ng
{

class EmscriptenWindowManager : public IWindowManager
{
    std::shared_ptr<IWindowManager> mEWindowManager;

public:
    EmscriptenWindowManager(std::shared_ptr<IWindowManager> eWindowManager)
        : mEWindowManager(std::move(eWindowManager))
    { }

    std::shared_ptr<IWindow> CreateWindow(const char* title,
                                          int width, int height,
                                          int x, int y,
                                          const VideoFlags& flags) override
    {
        return mEWindowManager->CreateWindow(title, width, height, x, y, flags);
    }

    bool PollEvent(WindowEvent& we) override
    {
        // TODO: Somehow grab events from javascript
        return false;
    }

    std::shared_ptr<IGLContext> CreateContext(const VideoFlags& flags,
                                              std::shared_ptr<IGLContext> sharedWith) override
    {
        return mEWindowManager->CreateContext(flags, sharedWith);
    }

    void SetCurrentContext(std::shared_ptr<IWindow> window,
                           std::shared_ptr<IGLContext> context) override
    {
        mEWindowManager->SetCurrentContext(window, context);
    }
};


std::shared_ptr<IWindowManager> CreateEmscriptenWindowManager(std::shared_ptr<IWindowManager> eWindowManager)
{
    return std::make_shared<EmscriptenWindowManager>(std::move(eWindowManager));
}

} // end namespace ng
