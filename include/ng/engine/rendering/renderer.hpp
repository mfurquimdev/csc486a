#ifndef NG_RENDERER_HPP
#define NG_RENDERER_HPP

#include <memory>

namespace ng
{

class IMesh;
class IRenderBatch;
class IWindowManager;
class IWindow;

class MeshID
{
public:
    const std::uint32_t ID;
};

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual std::shared_ptr<IRenderBatch> CreateRenderBatch() = 0;

    virtual MeshID AddMesh(std::unique_ptr<IMesh> mesh) = 0;
    virtual void RemoveMesh(MeshID meshID) = 0;
};

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window);

} // end namespace ng

#endif // NG_RENDERER_HPP
