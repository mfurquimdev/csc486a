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

public:
    GridMesh(std::shared_ptr<IRenderer> renderer);

    void Init(int numCols, int numRows, vec2 tileSize);

    RenderObjectPass PreUpdate(std::chrono::milliseconds,
                               RenderObjectNode&) override
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
