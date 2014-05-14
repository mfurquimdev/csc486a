#include "ng/engine/window.hpp"

#ifdef NG_USE_X11
#include "ng/engine/x11/xwindow.hpp"
#endif

namespace ng
{

std::unique_ptr<IWindow> CreateWindow(const char *title,
                                      int width, int height,
                                      int x, int y,
                                      const WindowFlags& flags)
{
#ifdef NG_USE_X11
    return CreateXWindow(title, width, height, x, y, flags);
#else
    static_assert(false, "No implementation of CreateWindow for this configuration.");
#endif
}

} // end namespace ng
