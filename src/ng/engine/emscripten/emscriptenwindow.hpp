#ifndef NG_EMSCRIPTENWINDOW_HPP
#define NG_EMSCRIPTENWINDOW_HPP

namespace ng
{

#include <memory>

namespace ng
{

class IWindowManager;

std::shared_ptr<IWindowManager> CreateEmscriptenWindowManager();

} // end namespace ng

#endif // NG_EMSCRIPTENWINDOW_HPP
