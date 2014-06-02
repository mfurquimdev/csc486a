#include "ng/engine/emscripten/emscriptenwindow.hpp"

#include "ng/engine/window/windowmanager.hpp"

#include "ng/engine/egl/eglwindow.hpp"

#include "ng/engine/window/window.hpp"

#include <emscripten.h>

namespace ng
{

class EmscriptenWindow : public IWindow
{
public:
    std::shared_ptr<IWindow> mEWindow;

    EmscriptenWindow(std::shared_ptr<IWindow> eWindow)
        : mEWindow(eWindow)
    { }

    void SwapBuffers() override
    {
        mEWindow->SwapBuffers();
    }

    void GetSize(int* width, int* height) const override
    {
        emscripten_get_canvas_size(width, height, nullptr);
    }

    void SetTitle(const char* title) override
    {
        // do nothing for now
    }

    const VideoFlags& GetVideoFlags() const override
    {
        return mEWindow->GetVideoFlags();
    }
};

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

        std::shared_ptr<IWindow> win = std::make_shared<EmscriptenWindow>(
                    mEWindowManager->CreateWindow(title, width, height, x, y, flags));
        emscripten_set_canvas_size(width, height);
        return win;
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
        const std::shared_ptr<IWindow>& eWindow = static_cast<const EmscriptenWindow&>(*window).mEWindow;
        mEWindowManager->SetCurrentContext(eWindow, context);
    }
};

std::shared_ptr<IWindowManager> CreateEmscriptenWindowManager(std::shared_ptr<IWindowManager> eWindowManager)
{
    return std::make_shared<EmscriptenWindowManager>(std::move(eWindowManager));
}

} // end namespace ng
