#ifndef NG_RENDERER_HPP
#define NG_RENDERER_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <memory>

namespace ng
{

class IWindowManager;
class IWindow;
class SceneGraph;

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void BeginFrame(vec3 clearColor) = 0;

    // submit a list of objects to render
    virtual void Render(const SceneGraph& scene) = 0;

    virtual void EndFrame() = 0;
};

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window);

} // end namespace ng

#endif // NG_RENDERER_HPP
