#ifndef NG_GEOMETRY_HPP
#define NG_GEOMETRY_HPP

#include "ng/engine/linearalgebra.hpp"

#include <limits>

namespace ng
{

template<class T>
struct Ray
{
    // Ray equation: p = Origin + Direction * t

    genType_base<T,3> Origin;
    genType_base<T,3> Direction;

    Ray() = default;

    Ray(genType_base<T,3> origin, genType_base<T,3> direction)
        : Origin(origin), Direction(direction)
    { }
};

template<class T>
struct Plane
{
    // Plane equation: Normal.x * x + Normal.y * y + Normal.z * z + d = 0
    genType_base<T,3> Normal;
    T D = 0;

    Plane() = default;

    Plane(genType_base<T,3> normal, T d)
        : Normal(normal)
        , D(d)
    { }

    Plane(genType_base<T,3> normal, genType_base<T,3> pointOnPlane)
        : Normal(normal)
        , D(-(dot(normal, pointOnPlane)))
    { }
};

template<class T>
struct AxisAlignedBoundingBox
{
    genType_base<T,3> Minimum;
    genType_base<T,3> Maximum;

    AxisAlignedBoundingBox() = default;

    AxisAlignedBoundingBox(genType_base<T,3> minimum, genType_base<T,3> maximum)
        : Minimum(minimum)
        , Maximum(maximum)
    { }

    genType_base<T,3> GetCenter() const
    {
        return {
            (Minimum.x + Maximum.x) / 2,
            (Minimum.y + Maximum.y) / 2,
            (Minimum.z + Maximum.z) / 2
        };
    }
};

template<class T>
bool RayPlaneIntersect(const Ray<T>& ray, const Plane<T>& plane, T tmin, T tmax, T& t)
{
    T denom = dot(plane.Normal, ray.Direction);

    if (std::abs(denom) <= std::numeric_limits<T>::epsilon())
    {
        return false;
    }

    T result = - (dot(plane.Normal, ray.Origin) + plane.D) / denom;

    if (result >= tmin && result <= tmax)
    {
        t = result;
        return true;
    }
    else
    {
        return false;
    }
}

} // end namespace ng

#endif // NG_GEOMETRY_HPP
