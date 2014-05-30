#ifndef NG_EGLWINDOW_HPP
#define NG_EGLWINDOW_HPP

namespace ng
{

#include <memory>

namespace ng
{

class IWindowManager;

std::shared_ptr<IWindowManager> CreateEWindowManager();

} // end namespace ng

#endif // NG_EGLWINDOW_HPP
