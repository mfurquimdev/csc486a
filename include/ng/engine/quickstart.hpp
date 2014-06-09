#ifndef NG_QUICKSTART_HPP
#define NG_QUICKSTART_HPP

#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/window.hpp"
#include "ng/engine/rendering/renderer.hpp"

// includes for convenience
#include "ng/engine/window/windowevent.hpp"
#include "ng/engine/rendering/shaderprogram.hpp"
#include "ng/engine/rendering/renderstate.hpp"
#include "ng/engine/math/linearalgebra.hpp"
#include "ng/engine/math/geometry.hpp"
#include "ng/engine/math/constants.hpp"

namespace ng
{

class QuickStart
{
public:
    std::shared_ptr<ng::IWindowManager> WindowManager;
    std::shared_ptr<ng::IWindow> Window;
    std::shared_ptr<ng::IRenderer> Renderer;

    QuickStart(int width = 640, int height = 480, const char* title = "QuickStart ng app");
};

} // end namespace ng

#endif // NG_QUICKSTART_HPP
