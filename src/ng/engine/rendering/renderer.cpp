#include "ng/engine/rendering/renderer.hpp"

#include "ng/engine/opengl/openglrenderer.hpp"

namespace ng
{

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window)
{
    bool useRenderingThread = true;

#ifdef NG_USE_EMSCRIPTEN
    useRenderingThread = false;
#endif

    return CreateOpenGLRenderer(
                std::move(windowManager),
                std::move(window),
                useRenderingThread);
}

} // end namespace ng
