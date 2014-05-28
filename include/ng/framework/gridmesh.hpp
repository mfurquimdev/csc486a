#ifndef NG_GRIDMESH_HPP
#define NG_GRIDMESH_HPP

#include "ng/framework/renderobject.hpp"

#include "ng/engine/uniform.hpp"
#include "ng/engine/linearalgebra.hpp"

#include <memory>
#include <map>

namespace ng
{

class IRenderer;
class IStaticMesh;
class IShaderProgram;
struct RenderState;

class GridMesh : public IRenderObject
{
    std::shared_ptr<IStaticMesh> mMesh;

    int mNumColumns = 0;
    int mNumRows = 0;
    vec2 mTileSize;

public:
    GridMesh(std::shared_ptr<IRenderer> renderer);

    void Init(int numColumns, int numRows, vec2 tileSize);

    vec3 GetNormal() const
    {
        return { 0.0f, 1.0f, 0.0f };
    }

    AxisAlignedBoundingBox<float> GetLocalBoundingBox() const override;

    RenderObjectPass PreUpdate(std::chrono::milliseconds deltaTime,
                               RenderObjectNode& node) override
    {
        return RenderObjectPass::Continue;
    }

    void PostUpdate(std::chrono::milliseconds,
                    RenderObjectNode&) override
    { }

    RenderObjectPass Draw(
            const std::shared_ptr<IShaderProgram>& program,
            const std::map<std::string, UniformValue>& uniforms,
            const RenderState& renderState) override;
};

} // end namespace gridmesh

#endif // NG_GRIDMESH_HPP
