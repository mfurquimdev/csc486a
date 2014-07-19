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
            float angleOfRotationInRadians)
    {
          return {{
                  axisOfRotation * std::sin(angleOfRotationInRadians / 2),
                  std::cos(angleOfRotationInRadians / 2)
              }};
    }

    Quaternion& operator*=(Quaternion other)
    {
        // evil but whatever
        vec<T,3> pv = reinterpret_cast<vec<T,3>&>(Components[0]);
        float ps = Components[3];
        vec<T,3> qv = reinterpret_cast<vec<T,3>&>(other.Components[0]);
        float qs = Components[3];

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
            1-2*y*y-2*z*z, 2*x*y-2*z*w,   2*x*z+2*y*w,
            2*x*y+2*z*w,   1-2*x*x-2*z*z, 2*y*z-2*x*w,
            2*x*z-2*y*w,   2*y*z+2*x*w,   1-2*x*x-2*y*y
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
Quaternion<T> conjugate(Quaternion<T> q)
{
    return {{
            -q.Components[0],
            -q.Components[1],
            -q.Components[2],
             q.Components[3]
        }};
}

template<class T>
T length(Quaternion<T> q)
{
    return length(q.Components);
}

template<class T>
Quaternion<T> normalize(Quaternion<T> q)
{
    return { normalize(q.Components) };
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
    Quaternion<T> qv(v, 0);
    return q * qv * inverse(q);
}

template<class T>
Quaternion<T> lerp(Quaternion<T> a, Quaternion<T> b, T percent)
{
    return normalize((1 - percent) * a + percent * b);
}

// this function is notoriously inefficient.
template<class T>
Quaternion<T> slerp(Quaternion<T> a, Quaternion<T> b, T percent)
{
    T th = std::acos(dot(a,b));
    T wa = std::sin((1 - percent) * th) / std::sin(th);
    T wb = std::sin(percent * th) / std::sin(th);
    return wa * a + wb * b;
}

using Quaternionf = Quaternion<float>;
using Quaterniond = Quaternion<double>;

} // end namespace ng

#endif // NG_QUATERNION_HPP
