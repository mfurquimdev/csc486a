#include "ng/engine/window/windowmanager.hpp"

#ifdef NG_USE_EGL
#include "ng/engine/egl/eglwindow.hpp"
#endif

#ifdef NG_USE_X11
#include "ng/engine/x11/xwindow.hpp"
#elif NG_USE_EMSCRIPTEN
#include "ng/engine/emscripten/emscriptenwindow.hpp"
#endif

namespace ng
{

std::shared_ptr<IWindowManager> CreateWindowManager()
{
#ifdef NG_USE_X11
    return CreateXWindowManager();
#elif NG_USE_EMSCRIPTEN
    return CreateEmscriptenWindowManager(CreateEWindowManager(EGL_DEFAULT_DISPLAY));
#else
    static_assert(false, "No implementation of CreateWindowManager for this configuration.");
#endif
}

} // end namespace ng
