#ifndef NG_XWINDOW_HPP
#define NG_XWINDOW_HPP

#include <memory>

namespace ng
{

class IWindowManager;

std::unique_ptr<IWindowManager> CreateXWindowManager();

} // end namespace ng

#endif // NG_XWINDOW_HPP
