#ifndef NG_RENDERER_HPP
#define NG_RENDERER_HPP

#include <memory>

namespace ng
{

class IWindowManager;
class IWindow;
class IStaticMesh;

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void Clear(bool color, bool depth, bool stencil) = 0;

    virtual void SwapBuffers() = 0;

    virtual std::shared_ptr<IStaticMesh> CreateStaticMesh() = 0;
};

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window);

} // end namespace ng

#endif // NG_RENDERER_HPP
