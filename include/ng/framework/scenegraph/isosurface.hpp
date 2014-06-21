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

    float operator()(float d) const
    {
        return std::exp(-StandardDeviation * std::pow(d, 2));
    }
};

class MetaballFilter
{
public:
    float MaxDistance;

    explicit MetaballFilter(float maxDistance)
        : MaxDistance(maxDistance)
    { }

    float operator()(float d) const
    {
        return d <= MaxDistance / 3.0f ? 1.0f - 3.0f * std::pow(d / MaxDistance, 2)
             : d <= MaxDistance        ? 3.0f / 2.0f * std::pow(1.0f - d / MaxDistance, 2)
             : 0.0f;
    }
};

class SoftObjectsFilter
{
public:
    float MaxDistance;

    explicit SoftObjectsFilter(float maxDistance)
        : MaxDistance(maxDistance)
    { }

    float operator()(float d) const
    {
        return 1.0f
             - 4.0f  / 9.0f * std::pow(d / MaxDistance, 6)
             + 17.0f / 9.0f * std::pow(d / MaxDistance, 4)
             - 22.0f / 9.0f * std::pow(d / MaxDistance, 2);
    }
};

class WyvillFilter
{
public:
    float MaxDistance;

    explicit WyvillFilter(float maxDistance)
        : MaxDistance(maxDistance)
    { }

    float operator()(float d) const
    {
        return std::pow(1.0f - std::pow(d / MaxDistance, 2), 3);
    }
};

class IsoPrimitive
{
    class IPrimitive
    {
    public:
        virtual ~IPrimitive() = default;

        virtual float GetDistanceToSkeleton(vec3 position) const = 0;
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

        float GetDistanceToSkeleton(vec3 position) const override
        {
            return DistanceToSkeleton(mPrimitive, position);
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
        return std::max(mFilter(mPrimitive->GetDistanceToSkeleton(position)),0.0f);
    }

    vec3 GetPointOnSkeleton() const
    {
        return mPrimitive->GetPointOnSkeleton();
    }
};

template<class T>
float DistanceToSkeleton(const Sphere<T>& sph, vec3 position)
{
    return length(position - sph.Center);
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
