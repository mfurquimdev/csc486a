#ifndef NG_LINEARALGEBRA_HPP
#define NG_LINEARALGEBRA_HPP

#include <array>

namespace ng
{

template<class T, std::size_t N>
struct gentype { };

template<class T>
struct gentype<T,1>
{
    union
    {
        struct
        {
            T x;
        };
        std::array<T,1> e;
    };

    gentype() = default;

    gentype(T xx)
        : x(xx)
    { }

    const T& operator[](std::size_t i) const { return e[i]; }
          T& operator[](std::size_t i)       { return e[i]; }

    operator float() const
    {
        return x;
    }

    operator float&()
    {
        return x;
    }
};

template<class T>
struct gentype<T,2>
{
    union
    {
        struct
        {
            T x,y;
        };
        std::array<T,2> e;
    };

    gentype() = default;

    gentype(T xx, T yy)
        : x(xx), y(yy)
    { }

    const T& operator[](std::size_t i) const { return e[i]; }
          T& operator[](std::size_t i)       { return e[i]; }
};

template<class T>
struct gentype<T,3>
{
    union
    {
        struct
        {
            T x,y,z;
        };
        std::array<T,3> e;
    };

    gentype() = default;

    gentype(T xx, T yy, T zz)
        : x(xx), y(yy), z(zz)
    { }

    gentype(gentype<T,2> v, T zz)
        : x(v.x), y(v.y), z(zz)
    { }

    gentype(T xx, gentype<T,2> v)
        : x(xx), y(v.x), z(v.y)
    { }

    const T& operator[](std::size_t i) const { return e[i]; }
          T& operator[](std::size_t i)       { return e[i]; }
};

template<class T>
struct gentype<T,4>
{
    union
    {
        struct
        {
            T x,y,z,w;
        };
        std::array<T,4> e;
    };

    gentype() = default;

    gentype(T xx, T yy, T zz, T ww)
        : x(xx), y(yy), z(zz), w(ww)
    { }

    gentype(gentype<T,3> v, T ww)
        : x(v.x), y(v.y), z(v.z), w(ww)
    { }

    gentype(T xx, gentype<T,3> v)
        : x(xx), y(v.x), z(v.y), w(v.z)
    { }

    gentype(gentype<T,2> v, gentype<T,2> u)
        : x(v.x), y(v.y), z(u.x), w(u.y)
    { }

    gentype(gentype<T,2> v, T zz, T ww)
        : x(v.x), y(v.y), z(zz), w(ww)
    { }

    gentype(T xx, T yy, gentype<T,2> v)
        : x(xx), y(yy), z(v.x), w(v.y)
    { }

    const T& operator[](std::size_t i) const { return e[i]; }
          T& operator[](std::size_t i)       { return e[i]; }
};

using vec1 = gentype<float,1>;
using vec2 = gentype<float,2>;
using vec3 = gentype<float,3>;
using vec4 = gentype<float,4>;

template<class T, std::size_t R, std::size_t C>
struct mat
{
    std::array<gentype<T,R>,C> e;

    const gentype<T,R>& operator[](std::size_t col) const { return e[col]; }
          gentype<T,R>& operator[](std::size_t col)       { return e[col]; }
};

using mat3 = mat<float,3,3>;
using mat4 = mat<float,4,4>;

} // end namespace ng

#endif // NG_LINEARALGEBRA_HPP
