#ifndef NG_LINESTRIP_HPP
#define NG_LINESTRIP_HPP

#include "ng/framework/scenegraph/renderobject.hpp"

#include "ng/engine/math/geometry.hpp"
#include "ng/engine/math/linearalgebra.hpp"

#include <vector>

namespace ng
{

class IStaticMesh;
class IRenderer;

class LineStrip : public IRenderObject
{
    std::shared_ptr<IStaticMesh> mMesh;

    bool mIsMeshDirty = true;
    std::vector<vec3> mPoints;
    AxisAlignedBoundingBox<float> mBoundingBox;

public:
    LineStrip(std::shared_ptr<IRenderer> renderer);

    void UpdatePoints(const vec3* points, std::size_t numPoints);

    void UpdatePoints(std::vector<vec3> points);

    AxisAlignedBoundingBox<float> GetLocalBoundingBox() const override;

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

} // end namespace ng

#endif // NG_LINESTRIP_HPP
