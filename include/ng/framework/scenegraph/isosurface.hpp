#ifndef NG_ISOSURFACE_HPP
#define NG_ISOSURFACE_HPP

#include "ng/framework/scenegraph/renderobject.hpp"
#include "ng/engine/math/linearalgebra.hpp"
#include "ng/engine/util/memory.hpp"

#include <memory>
#include <functional>
#include <vector>

namespace ng
{

class IRenderer;
class IStaticMesh;

class BlobbyFilter
{
public:
    float StandardDeviation;

    explicit BlobbyFilter(float standardDeviation)
        : StandardDeviation(standardDeviation)
    { }

    float operator()(float d2) const
    {
        return std::exp(-StandardDeviation * d2);
    }
};

class MetaballFilter
{
public:
    float MaxDistanceSquared;

    explicit MetaballFilter(float maxDistance)
        : MaxDistanceSquared(maxDistance*maxDistance)
    { }

    float operator()(float d2) const
    {
        float ratio = d2 / MaxDistanceSquared;

        return d2 <= MaxDistanceSquared / 9.0f ? 1.0f - 3.0f * ratio
             : d2 <= MaxDistanceSquared        ? 3.0f / 2.0f * std::pow(1.0f - std::sqrt(ratio), 2)
             : 0.0f;
    }
};

class SoftObjectsFilter
{
public:
    float MaxDistanceSquared;

    explicit SoftObjectsFilter(float maxDistance)
        : MaxDistanceSquared(maxDistance*maxDistance)
    { }

    float operator()(float d2) const
    {
        float ratio = d2 / MaxDistanceSquared;

        return 1.0f
             - 4.0f  / 9.0f * ratio * ratio * ratio
             + 17.0f / 9.0f * ratio * ratio
             - 22.0f / 9.0f * ratio;
    }
};

class WyvillFilter
{
public:
    float MaxDistanceSquared;

    explicit WyvillFilter(float maxDistance)
        : MaxDistanceSquared(maxDistance * maxDistance)
    { }

    float operator()(float d2) const
    {
        float ratio = d2 / MaxDistanceSquared;
        float oneMinusRatio = 1.0f - ratio;
        return oneMinusRatio * oneMinusRatio * oneMinusRatio;
    }
};

class IsoPrimitive
{
    class IPrimitive
    {
    public:
        virtual ~IPrimitive() = default;

        virtual float GetDistanceSquaredToSkeleton(vec3 position) const = 0;
        virtual vec3 GetPointOnSkeleton() const = 0;
    };

    template<class PrimitiveT>
    class PrimitiveImpl : public IPrimitive
    {
        PrimitiveT mPrimitive;

    public:
        PrimitiveImpl(PrimitiveT primitive)
            : mPrimitive(std::move(primitive))
        { }

        float GetDistanceSquaredToSkeleton(vec3 position) const override
        {
            return DistanceSquaredToSkeleton(mPrimitive, position);
        }

        vec3 GetPointOnSkeleton() const override
        {
            return PointOnSkeleton(mPrimitive);
        }
    };

    std::unique_ptr<IPrimitive> mPrimitive;
    std::function<float(float)> mFilter;

public:
    template<class PrimitiveT, class FilterT>
    IsoPrimitive(PrimitiveT primitive,
                 FilterT filter)
        : mPrimitive(ng::make_unique<PrimitiveImpl<typename std::remove_reference<PrimitiveT>::type>>(primitive))
        , mFilter(std::move(filter))
    { }

    float GetFieldValue(vec3 position) const
    {
        return std::max(mFilter(mPrimitive->GetDistanceSquaredToSkeleton(position)),0.0f);
    }

    vec3 GetPointOnSkeleton() const
    {
        return mPrimitive->GetPointOnSkeleton();
    }
};

template<class T>
float DistanceSquaredToSkeleton(const Sphere<T>& sph, vec3 position)
{
    vec3 diff = position - sph.Center;
    return dot(diff,diff);
}

template<class T>
vec3 PointOnSkeleton(const Sphere<T>& sph)
{
    return sph.Center;
}

class IsoSurface : public IRenderObject
{
    std::shared_ptr<IStaticMesh> mMesh;
    AxisAlignedBoundingBox<float> mBoundingBox;

public:
    IsoSurface(std::shared_ptr<IRenderer> renderer);

    void Polygonize(const std::vector<IsoPrimitive>& primitives,
                    float isoValue,
                    float voxelSize);

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
