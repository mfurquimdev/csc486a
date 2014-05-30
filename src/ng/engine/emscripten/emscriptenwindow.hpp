#ifndef NG_EMSCRIPTENWINDOW_HPP
#define NG_EMSCRIPTENWINDOW_HPP

namespace ng
{

#include <memory>

namespace ng
{

class IWindowManager;

// must pass in a WindowManager created by CreateEWindowManager()
std::shared_ptr<IWindowManager> CreateEmscriptenWindowManager(std::shared_ptr<IWindowManager> eWindowManager);

} // end namespace ng

#endif // NG_EMSCRIPTENWINDOW_HPP
