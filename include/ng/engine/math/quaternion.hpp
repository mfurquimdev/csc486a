#ifndef NG_QUATERNION_HPP
#define NG_QUATERNION_HPP

#include "ng/engine/math/linearalgebra.hpp"

namespace ng
{

template<class T>
class Quaternion
{
public:
    vec<T,4> Components;

    static Quaternion FromAxisAndRotation(
            vec<T,3> axisOfRotation,
            Radians<T> angleOfRotation)
    {
        Quaternion q;

        vec<T,3> normalizedAxis = normalize(axisOfRotation);
        T s = std::sin(angleOfRotation.Value / 2);
        q.Components.x = normalizedAxis.x * s;
        q.Components.y = normalizedAxis.x * s;
        q.Components.z = normalizedAxis.x * s;
        q.Components.w = std::cos(angleOfRotation.Value / 2);

        return q;
    }

    static Quaternion FromComponents(vec<T,4> components)
    {
        Quaternion q;
        q.Components = components;
        return q;
    }

    static Quaternion FromComponents(T x,  T y, T z, T w)
    {
        Quaternion q;
        q.Components.x = x;
        q.Components.y = y;
        q.Components.z = z;
        q.Components.w = w;
        return q;
    }

    Quaternion& operator*=(Quaternion other)
    {
        vec<T,3> pv(Components);

        float ps = Components[3];

        vec<T,3> qv(other.Components);

        float qs = other.Components[3];

        Components = vec<T,4>(
                    ps * qv + qs * pv + cross(pv,qv),
                    ps * qs - dot(pv,qv));

        return *this;
    }

    Quaternion& operator*=(T s)
    {
        Components *= s;
        return *this;
    }

    Quaternion& operator/=(T s)
    {
        Components /= s;
        return *this;
    }

    explicit operator mat<T,3,3>() const
    {
        T x = Components[0];
        T y = Components[1];
        T z = Components[2];
        T w = Components[3];
        return {
            1-2*y*y-2*z*z, 2*x*y+2*z*w,   2*x*z-2*y*w,
            2*x*y-2*z*w,   1-2*x*x-2*z*z, 2*y*z+2*x*w,
            2*x*z+2*y*w,   2*y*z-2*x*w,   1-2*x*x-2*y*y
        };
    }
};

template<class T>
Quaternion<T> operator*(Quaternion<T> p, Quaternion<T> q)
{
    return p *= q;
}

template<class T>
Quaternion<T> operator*(Quaternion<T> q, T s)
{
    return q *= s;
}

template<class T>
Quaternion<T> operator*(T s, Quaternion<T> q)
{
    return q *= s;
}

template<class T>
Quaternion<T> operator/(Quaternion<T> q, T s)
{
    return q /= s;
}

template<class T>
T dot(Quaternion<T> q, Quaternion<T> p)
{
    return dot(q.Components, p.Components);
}

template<class T>
Quaternion<T> conjugate(Quaternion<T> q)
{
    return Quaternion<T>::FromComponents(
            -q.Components[0],
            -q.Components[1],
            -q.Components[2],
             q.Components[3]
        );
}

template<class T>
T length(Quaternion<T> q)
{
    return length(q.Components);
}

template<class T>
Quaternion<T> normalize(Quaternion<T> q)
{
    return Quaternion<T>::FromComponents(normalize(q.Components));
}

// note: if you know that the quaternion is a unit quaternion,
//       then conjugate(q) == inverse(q)
//       so the conjugate is a much more efficient choice...
template<class T>
Quaternion<T> inverse(Quaternion<T> q)
{
    return conjugate(q) / (length(q) * length(q));
}

// note: if you know that the quaternion is a unit quaternion,
//       then conjugate(q) == inverse(q)
//       so rotation can be computed more efficiently with q * v * conjugate(q)
template<class T>
Quaternion<T> rotate(Quaternion<T> q, vec<T,3> v)
{
    Quaternion<T> qv(Quaternion<T>::FromComponents(vec<T,4>(v, T(0))));
    return q * qv * inverse(q);
}

template<class T>
Quaternion<T> lerp(Quaternion<T> a, Quaternion<T> b, T percent)
{
    return normalize(Quaternion<T>::FromComponents(
                     (1 - percent) * a.Components
                   + percent * b.Components));
}

// this function is notoriously inefficient.
template<class T>
Quaternion<T> slerp(Quaternion<T> a, Quaternion<T> b, T percent)
{
    Quaternion<T> na = normalize(a);
    Quaternion<T> nb = normalize(b);
    T d = dot(na,nb);
    if (d > -0.95f && d < 0.95f)
    {
        T th = std::acos(d);
        T wa = std::sin((1 - percent) * th) / std::sin(th);
        T wb = std::sin(percent * th) / std::sin(th);
        return Quaternion<T>::FromComponents(
                                wa * na.Components
                              + wb * nb.Components);
    }
    else
    {
        // small angle. use lerp instead.
        return lerp(a,b,percent);
    }
}

using Quaternionf = Quaternion<float>;
using Quaterniond = Quaternion<double>;

} // end namespace ng

#endif // NG_QUATERNION_HPP
