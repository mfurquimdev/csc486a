#ifndef NG_IMPLCITSURFACEMESH_HPP
#define NG_IMPLCITSURFACEMESH_HPP

#include "ng/engine/rendering/mesh.hpp"

#include "ng/engine/math/geometry.hpp"
#include "ng/engine/math/linearalgebra.hpp"

#include "ng/engine/util/memory.hpp"

#include <functional>
#include <memory>
#include <vector>
#include <cmath>

namespace ng
{

template<unsigned int Nbits=5>
class WyvillHash
{
public:
    static constexpr unsigned int NBits = Nbits;
    static constexpr unsigned int BMask = (1 << NBits) - 1;

    constexpr std::uint16_t operator()(ivec3 i) const
    {
        // takes 5 least significant bits of x,y,z and combines them
        // result: 0xxxxxyyyyyzzzzz
        return ((((i.x & BMask) << NBits) | (i.y & BMask)) << NBits) | (i.z & BMask);
    }
};

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
             : d2 <= MaxDistanceSquared ? 3.0f / 2.0f * std::pow(1.0f - std::sqrt(ratio), 2)
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
             - 4.0f / 9.0f * ratio * ratio * ratio
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

class ImplicitSurfacePrimitive
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
    ImplicitSurfacePrimitive(PrimitiveT primitive, FilterT filter)
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
float DistanceSquaredToSkeleton(const Point<T>& pt, vec3 position)
{
    vec3 diff = position - pt.Position;
    return dot(diff,diff);
}

template<class T>
vec3 PointOnSkeleton(const Point<T>& pt)
{
    return pt.Position;
}

class ImplicitSurfaceMesh : public IMesh
{
    const std::vector<ImplicitSurfacePrimitive> mPrimitives;
    const float mIsoValue;
    const float mVoxelSize;

public:
    class Vertex
    {
    public:
        vec3 Position;
        vec3 Normal;
    };

    ImplicitSurfaceMesh(
            std::vector<ImplicitSurfacePrimitive> primitives,
            float isoValue,
            float voxelSize);

    VertexFormat GetVertexFormat() const override;

    std::size_t GetMaxVertexBufferSize() const override;
    std::size_t GetMaxIndexBufferSize() const override;

    std::size_t WriteVertices(void* buffer) const override;
    std::size_t WriteIndices(void* buffer) const override;
};

} // end namespace ng

#endif // NG_IMPLCITSURFACEMESH_HPP
