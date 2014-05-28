#ifndef NG_LINESTRIP_HPP
#define NG_LINESTRIP_HPP

#include "ng/framework/renderobject.hpp"

#include "ng/engine/geometry.hpp"
#include "ng/engine/linearalgebra.hpp"

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

    void Reset();

    void AddPoint(vec3 point);

    void RemovePoint(std::vector<vec3>::const_iterator which);

    void SetPoint(std::vector<vec3>::const_iterator which, vec3 value);

    const std::vector<vec3>& GetPoints() const
    {
        return mPoints;
    }

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
