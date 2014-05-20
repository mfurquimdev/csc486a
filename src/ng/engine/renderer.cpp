#include "ng/engine/renderer.hpp"

#include "ng/engine/opengl/glrenderer.hpp"

namespace ng
{

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window)
{
    return std::make_shared<OpenGLRenderer>(std::move(windowManager), std::move(window));
}

} // end namespace ng
