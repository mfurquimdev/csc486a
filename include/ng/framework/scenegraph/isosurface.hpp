#ifndef NG_ISOSURFACE_HPP
#define NG_ISOSURFACE_HPP

#include "ng/framework/scenegraph/renderobject.hpp"
#include "ng/engine/math/linearalgebra.hpp"

#include <memory>
#include <functional>

namespace ng
{

class IRenderer;
class IStaticMesh;

class IsoSurface : public IRenderObject
{
    std::shared_ptr<IStaticMesh> mMesh;
    AxisAlignedBoundingBox<float> mBoundingBox;

public:
    IsoSurface(std::shared_ptr<IRenderer> renderer);

    void Polygonize(std::function<float(vec3)> fieldFunction,
                    float isoValue,
                    float voxelSize,
                    std::vector<ivec3> seedVoxels);

    AxisAlignedBoundingBox<float> GetLocalBoundingBox() const override
    {
        return mBoundingBox;
    }

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

#endif // NG_ISOSURFACE_HPP
