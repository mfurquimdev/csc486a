#ifndef NG_XWINDOW_HPP
#define NG_XWINDOW_HPP

#include <memory>

namespace ng
{

class IWindow;

std::unique_ptr<IWindow> CreateXWindow(const char* title, int x, int y, int width, int height);

} // end namespace ng

#endif // NG_XWINDOW_HPP
