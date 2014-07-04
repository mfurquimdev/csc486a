#ifndef NG_GLRENDERER_HPP
#define NG_GLRENDERER_HPP

#include <memory>

namespace ng
{

class IRenderer;
class IWindowManager;
class IWindow;

std::shared_ptr<IRenderer> CreateOpenGLRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window,
        bool useRenderingThread);

} // end namespace ng

#endif // NG_GLRENDERER_HPP
