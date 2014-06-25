#ifndef NG_RENDERER_HPP
#define NG_RENDERER_HPP

#include <memory>

namespace ng
{

class IWindowManager;
class IWindow;
class IRenderBatch;

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual std::shared_ptr<IRenderBatch> CreateRenderBatch() = 0;
};

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window);

} // end namespace ng

#endif // NG_RENDERER_HPP
