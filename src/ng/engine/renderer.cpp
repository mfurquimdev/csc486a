#include "ng/engine/rendering/renderer.hpp"

#include "ng/engine/opengl/glrenderer.hpp"

namespace ng
{

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window)
{
    RenderingMode renderingMode;

#ifdef NG_USE_EMSCRIPTEN
    renderingMode = RenderingMode::Synchronous;
#else
    renderingMode = RenderingMode::Asynchronous;
#endif

    return std::make_shared<OpenGLRenderer>(std::move(windowManager), std::move(window), renderingMode);
}

} // end namespace ng
