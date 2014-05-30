#ifndef NG_GEOMETRY_HPP
#define NG_GEOMETRY_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <limits>

namespace ng
{

template<class T>
struct Ray
{
    // Ray equation: p = Origin + Direction * t

    vec<T,3> Origin;
    vec<T,3> Direction;

    Ray() = default;

    Ray(vec<T,3> origin, vec<T,3> direction)
        : Origin(origin), Direction(direction)
    { }
};

template<class T>
struct Plane
{
    // Plane equation: Normal.x * x + Normal.y * y + Normal.z * z + d = 0
    vec<T,3> Normal;
    T D = 0;

    Plane() = default;

    Plane(vec<T,3> normal, T d)
        : Normal(normal)
        , D(d)
    { }

    Plane(vec<T,3> normal, vec<T,3> pointOnPlane)
        : Normal(normal)
        , D(-(dot(normal, pointOnPlane)))
    { }
};

template<class T>
struct AxisAlignedBoundingBox
{
    vec<T,3> Minimum;
    vec<T,3> Maximum;

    AxisAlignedBoundingBox() = default;

    AxisAlignedBoundingBox(vec<T,3> minimum, vec<T,3> maximum)
        : Minimum(minimum)
        , Maximum(maximum)
    { }

    vec<T,3> GetCenter() const
    {
        return {
            (Minimum.x + Maximum.x) / 2,
            (Minimum.y + Maximum.y) / 2,
            (Minimum.z + Maximum.z) / 2
        };
    }

    void AddPoint(vec3 point)
    {
        if (point.x < Minimum.x) Minimum.x = point.x;
        if (point.y < Minimum.y) Minimum.y = point.y;
        if (point.z < Minimum.z) Minimum.z = point.z;

        if (point.x > Maximum.x) Maximum.x = point.x;
        if (point.y > Maximum.y) Maximum.y = point.y;
        if (point.z > Maximum.z) Maximum.z = point.z;
    }
};

template<class T>
struct Sphere
{
    vec<T,3> Center;
    T Radius = 0;

    Sphere() = default;

    Sphere(vec<T,3> center, T radius)
        : Center(center)
        , Radius(radius)
    { }
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

// returns closest collision
template<class T>
bool RaySphereIntersect(const Ray<T>& ray, const Sphere<T>& sphere, T tmin, T tmax, T& t)
{
    vec3 toOrigin = ray.Origin - sphere.Center;

    T a = dot(ray.Direction, ray.Direction);
    T b = 2 * dot(ray.Direction, toOrigin);
    T c = dot(toOrigin, toOrigin) - sphere.Radius * sphere.Radius;

    T discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
    {
        // no solution
        return false;
    }

    // one or two solutions
    discriminant = sqrt(discriminant);

    // start by seeing if the closer result is acceptable
    T result = (- b - discriminant)  / (2 * a);

    if (result < tmin)
    {
        // try the further result instead of the closer one
        result = (- b + discriminant) / (2 * a);
    }

    if (result < tmin || result > tmax)
    {
        // no solution in the valid range
        return false;
    }

    // otherwise, we have a valid result.
    t = result;
    return true;
}

} // end namespace ng

#endif // NG_GEOMETRY_HPP
