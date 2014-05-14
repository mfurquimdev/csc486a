#ifndef NG_XWINDOW_HPP
#define NG_XWINDOW_HPP

#include <memory>

namespace ng
{

class IWindow;
class WindowFlags;

std::unique_ptr<IWindow> CreateXWindow(const char* title,
                                       int width, int height,
                                       int x, int y,
                                       const WindowFlags& flags);

} // end namespace ng

#endif // NG_XWINDOW_HPP
