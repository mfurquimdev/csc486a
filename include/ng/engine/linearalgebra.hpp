#ifndef NG_LINEARALGEBRA_HPP
#define NG_LINEARALGEBRA_HPP

#include <cstddef>

namespace ng
{

template<class T, std::size_t N>
struct vec { };

template<class T>
struct vec<T,2>
{
    union
    {
        struct
        {
            T x,y;
        };
        T e[2];
    };
};

template<class T>
struct vec<T,3>
{
    union
    {
        struct
        {
            T x,y,z;
        };
        T e[3];
    };
};

template<class T>
struct vec<T,4>
{
    union
    {
        struct
        {
            T x,y,z,w;
        };
        T e[4];
    };
};

using vec2 = vec<float,2>;
using vec3 = vec<float,3>;
using vec4 = vec<float,4>;

template<class T, std::size_t R, std::size_t C>
struct mat
{
    T e[C][R];
};

using mat3 = mat<float,3,3>;
using mat4 = mat<float,4,4>;

} // end namespace ng

#endif // NG_LINEARALGEBRA_HPP
