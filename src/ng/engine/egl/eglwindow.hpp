#ifndef NG_EGLWINDOW_HPP
#define NG_EGLWINDOW_HPP

#include <EGL/egl.h>

#include <memory>

namespace ng
{

class IWindowManager;

std::shared_ptr<IWindowManager> CreateEWindowManager(EGLNativeWindowType nativeWindow);

} // end namespace ng

#endif // NG_EGLWINDOW_HPP
