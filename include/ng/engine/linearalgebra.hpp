#ifndef NG_LINEARALGEBRA_HPP
#define NG_LINEARALGEBRA_HPP

#include <cmath>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <stdexcept>

namespace ng
{

template<class T, std::size_t N>
struct genType_storage;

template<class T, std::size_t N>
struct genType_base : public genType_storage<T,N>
{
    using genType_storage<T,N>::genType_storage;

    genType_base() = default;

    template<class U, std::size_t M>
    explicit genType_base(genType_base<U,M> other)
    {
        static_assert(M >= N, "Can only create smaller vectors from bigger vectors");

        for (std::size_t i = 0; i < N; i++)
        {
            (*this)[i] = other[i];
        }
    }

    const T& operator[](std::size_t i) const { return *(&(genType_storage<T,N>::x) + i); }
          T& operator[](std::size_t i)       { return *(&(genType_storage<T,N>::x) + i); }

    genType_base& operator+=(genType_base other)
    {
        for (std::size_t i = 0; i < N; i++)
        {
            (*this)[i] += other[i];
        }
        return *this;
    }

    genType_base& operator-=(genType_base other)
    {
        for (std::size_t i = 0; i < N; i++)
        {
            (*this)[i] -= other[i];
        }
        return *this;
    }

    genType_base& operator*=(genType_base other)
    {
        for (std::size_t i = 0; i < N; i++)
        {
            (*this)[i] *= other[i];
        }
        return *this;
    }

    genType_base& operator*=(T s)
    {
        for (std::size_t i = 0; i < N; i++)
        {
            (*this)[i] *= s;
        }
        return *this;
    }

    genType_base& operator/=(genType_base other)
    {
        for (std::size_t i = 0; i < N; i++)
        {
            (*this)[i] /= other[i];
        }
        return *this;
    }

    genType_base& operator/=(T s)
    {
        for (std::size_t i = 0; i < N; i++)
        {
            (*this)[i] /= s;
        }
        return *this;
    }
};

template<class T, std::size_t N>
genType_base<T,N> operator+(genType_base<T,N> lhs, genType_base<T,N> rhs)
{
    lhs += rhs;
    return lhs;
}

template<class T, std::size_t N>
genType_base<T,N> operator-(genType_base<T,N> lhs, genType_base<T,N> rhs)
{
    lhs -= rhs;
    return lhs;
}

template<class T, std::size_t N>
genType_base<T,N> operator*(genType_base<T,N> lhs, genType_base<T,N> rhs)
{
    lhs *= rhs;
    return lhs;
}

template<class T, std::size_t N>
genType_base<T,N> operator*(genType_base<T,N> lhs, T rhs)
{
    lhs *= rhs;
    return lhs;
}

template<class T, std::size_t N>
genType_base<T,N> operator*(T lhs, genType_base<T,N> rhs)
{
    rhs *= lhs;
    return rhs;
}

template<class T, std::size_t N>
genType_base<T,N> operator/(genType_base<T,N> lhs, genType_base<T,N> rhs)
{
    lhs /= rhs;
    return lhs;
}

template<class T, std::size_t N>
genType_base<T,N> operator/(genType_base<T,N> lhs, T rhs)
{
    lhs /= rhs;
    return lhs;
}

template<class T, std::size_t N>
bool operator==(genType_base<T,N> lhs, genType_base<T,N> rhs)
{
    for (std::size_t i = 0; i < N; i++)
    {
        if (lhs[i] != rhs[i])
        {
            return false;
        }
    }

    return true;
}

template<class T, std::size_t N>
bool operator!=(genType_base<T,N> lhs, genType_base<T,N> rhs)
{
    return !(lhs == rhs);
}

template<class T>
struct genType_storage<T,1>
{
    T x;

    genType_storage()
        : x(0)
    { }

    explicit genType_storage(T xx)
        : x(xx)
    { }

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
struct genType_storage<T,2>
{
    T x,y;

    genType_storage()
        : x(0), y(0)
    { }

    genType_storage(T s)
        : x(s), y(s)
    { }

    genType_storage(T xx, T yy)
        : x(xx), y(yy)
    { }
};

template<class T>
struct genType_storage<T,3>
{
    T x,y,z;

    genType_storage()
        : x(0), y(0), z(0)
    { }

    explicit genType_storage(T s)
        : x(s), y(s), z(s)
    { }

    genType_storage(T xx, T yy, T zz)
        : x(xx), y(yy), z(zz)
    { }

    genType_storage(genType_base<T,2> v, T zz)
        : x(v.x), y(v.y), z(zz)
    { }

    genType_storage(T xx, genType_base<T,2> v)
        : x(xx), y(v.x), z(v.y)
    { }
};

template<class T>
struct genType_storage<T,4>
{
    T x,y,z,w;

    genType_storage()
        : x(0), y(0), z(0), w(1)
    { }

    explicit genType_storage(T s)
        : x(s), y(s), z(s), w(s)
    { }

    genType_storage(T xx, T yy, T zz, T ww)
        : x(xx), y(yy), z(zz), w(ww)
    { }

    genType_storage(genType_base<T,3> v, T ww)
        : x(v.x), y(v.y), z(v.z), w(ww)
    { }

    genType_storage(T xx, genType_base<T,3> v)
        : x(xx), y(v.x), z(v.y), w(v.z)
    { }

    genType_storage(genType_base<T,2> u, genType_base<T,2> v)
        : x(u.x), y(u.y), z(v.x), w(v.y)
    { }

    genType_storage(genType_base<T,2> v, T zz, T ww)
        : x(v.x), y(v.y), z(zz), w(ww)
    { }

    genType_storage(T xx, T yy, genType_base<T,2> v)
        : x(xx), y(yy), z(v.x), w(v.y)
    { }
};

template<std::size_t N>
using genType = genType_base<float,N>;

template<std::size_t N>
using genIType = genType_base<std::int32_t,N>;

template<std::size_t N>
using genUType = genType_base<std::uint32_t,N>;

template<std::size_t N>
using genBType = genType_base<bool,N>;

template<std::size_t N>
using genDType = genType_base<double,N>;

template<class T, std::size_t N>
typename std::enable_if<std::is_same<T,float>::value || std::is_same<T,double>::value,
T>::type dot(genType_base<T,N> u, genType_base<T,N> v)
{
    T sum = 0;
    for (std::size_t i = 0; i < N; i++)
    {
        sum += u[i] * v[i];
    }
    return sum;
}

template<class T>
typename std::enable_if<std::is_same<T,float>::value || std::is_same<T,double>::value,
genType_base<T,3>>::type cross(genType_base<T,3> u, genType_base<T,3> v)
{
    return {
             u[1] * v[2] - v[1] * u[2],
             u[2] * v[0] - v[2] * u[0],
             u[0] * v[1] - v[0] * u[1]
    };
}

template<class T, std::size_t N>
typename std::enable_if<std::is_same<T,float>::value || std::is_same<T,double>::value,
T>::type length(genType_base<T,N> v)
{
    return std::sqrt(dot(v,v));
}

template<class T, std::size_t N>
typename std::enable_if<std::is_same<T,float>::value || std::is_same<T,double>::value,
genType_base<T,N>>::type normalize(genType_base<T,N> v)
{
    return v / length(v);
}

using vec1 = genType<1>;
using vec2 = genType<2>;
using vec3 = genType<3>;
using vec4 = genType<4>;

using ivec1 = genIType<1>;
using ivec2 = genIType<2>;
using ivec3 = genIType<3>;
using ivec4 = genIType<4>;

using uvec1 = genUType<1>;
using uvec2 = genUType<2>;
using uvec3 = genUType<3>;
using uvec4 = genUType<4>;

using bvec1 = genBType<1>;
using bvec2 = genBType<2>;
using bvec3 = genBType<3>;
using bvec4 = genBType<4>;

using dvec1 = genDType<1>;
using dvec2 = genDType<2>;
using dvec3 = genDType<3>;
using dvec4 = genDType<4>;

template<class T, std::size_t C, std::size_t R>
struct mat_storage
{
    genType_base<T,R> e[C];

    mat_storage() = default;

    mat_storage(T c11, T c12, T c13,
                T c21, T c22, T c23,
                T c31, T c32, T c33)
        : e {
              { c11, c12, c13 },
              { c21, c22, c23 },
              { c31, c32, c33 },
          }
    { }

    mat_storage(T c11, T c12, T c13, T c14,
                T c21, T c22, T c23, T c24,
                T c31, T c32, T c33, T c34,
                T c41, T c42, T c43, T c44)
        : e {
              { c11, c12, c13, c14 },
              { c21, c22, c23, c24 },
              { c31, c32, c33, c34 },
              { c41, c42, c43, c44 },
          }
    { }

    mat_storage(genType_base<T,R> col1,
                genType_base<T,R> col2,
                genType_base<T,R> col3)
        : e {
              { col1.x, col1.y, col1.z },
              { col2.x, col2.y, col2.z },
              { col3.x, col3.y, col3.z },
          }
    { }

    mat_storage(genType_base<T,R> col1,
                genType_base<T,R> col2,
                genType_base<T,R> col3,
                genType_base<T,R> col4)
        : e {
              { col1.x, col1.y, col1.z, col1.w },
              { col2.x, col2.y, col2.z, col2.w },
              { col3.x, col3.y, col3.z, col3.w },
              { col4.x, col4.y, col4.z, col4.w }
          }
    { }

    explicit mat_storage(const genType_base<T,R> (&ee)[C])
        : e(ee)
    { }

    const genType_base<T,R>& operator[](std::size_t col) const { return e[col]; }
          genType_base<T,R>& operator[](std::size_t col)       { return e[col]; }
};

template<class T, std::size_t C, std::size_t R>
struct mat_base : mat_storage<T,C,R>
{
    using mat_storage<T,C,R>::mat_storage;
    using mat_storage<T,C,R>::e;

    // TODO: Figure out how a non-square matrix default constructor should be formed.
    mat_base();
};

template<class T, std::size_t N>
struct mat_base<T,N,N> : mat_storage<T,N,N>
{
    using mat_storage<T,N,N>::mat_storage;
    using mat_storage<T,N,N>::e;

    mat_base()
    {
        for (std::size_t c = 0; c < N; c++)
        {
            for (std::size_t r = 0; r < N; r++)
            {
                e[c][r] = c == r ? 1 : 0;
            }
        }
    }

    mat_base(T diagonal)
    {
        for (std::size_t c = 0; c < N; c++)
        {
            for (std::size_t r = 0; r < N; r++)
            {
                e[c][r] = c == r ? diagonal : 0;
            }
        }
    }
};

template<class T, std::size_t C, std::size_t R>
struct mat : mat_base<T,C,R>
{
    using mat_base<T,C,R>::mat_base;
    using mat_base<T,C,R>::e;

    mat() = default;

    template<class U, std::size_t C2, std::size_t R2>
    explicit mat(mat<U,C2,R2> other)
    {
        for (std::size_t c = 0; c < std::min(C,C2); c++)
        {
            for (std::size_t r = 0; r < std::min(R,R2); r++)
            {
                e[c][r] = other[c][r];
            }
        }
    }

    mat& operator*=(mat<T,C,R> other)
    {
        e = ((*this) * other).e;
        return *this;
    }

    mat& operator*=(T s)
    {
        for (std::size_t c = 0; c < C; c++)
        {
            for (std::size_t r = 0; r < R; r++)
            {
                e[c][r] *= s;
            }
        }

        return *this;
    }

    mat& operator/=(T s)
    {
        return (*this) *= (1 / s);
    }
};

template<class T, std::size_t C1, std::size_t R1, std::size_t C2>
mat<T,C2,R1> operator*(mat<T,C1,R1> lhs, mat<T,C2,C1> rhs)
{
    mat<T,C2,R1> tmp;
    for (std::size_t r = 0; r < R1; r++)
    {
        for (std::size_t c = 0; c < C2; c++)
        {
            T d = 0;
            for (std::size_t i = 0; i < C1; i++)
            {
                d += lhs[i][r] * rhs[c][i];
            }
            tmp[c][r] = d;
        }
    }

    return tmp;
}

template<class T, std::size_t C, std::size_t R>
mat<T,C,R> operator*(mat<T,C,R> m, T s)
{
    return m *= s;
}

template<class T, std::size_t C, std::size_t R>
mat<T,C,R> operator*(T s, mat<T,C,R> m)
{
    return m *= s;
}

template<class T, std::size_t C, std::size_t R>
mat<T,C,R> operator/(mat<T,C,R> m, T s)
{
    return m /= s;
}

template<class T, std::size_t C, std::size_t R>
genType_base<T,R> operator*(mat<T,C,R> m, genType_base<T,C> v)
{
    genType_base<T,R> tmp;
    for (std::size_t r = 0; r < R; r++)
    {
        T d = 0;
        for (std::size_t i = 0; i < C; i++)
        {
            d += m[i][r] * v[i];
        }
        tmp[r] = d;
    }

    return tmp;
}

template<class T, std::size_t C, std::size_t R>
genType_base<T,R>& operator*=(genType_base<T,R>& v, mat<T,C,R> m)
{
    v = m * v;
    return v;
}

template<class T, std::size_t C, std::size_t R>
mat<T,C,R> transpose(mat<T,C,R> m)
{
    mat<T,R,C> result;
    for (std::size_t c = 0; c < C; c++)
    {
        for (std::size_t r = 0; r < R; r++)
        {
            result[r][c] = m[c][r];
        }
    }
    return result;
}

template<class T>
typename std::enable_if<std::is_same<T,float>::value || std::is_same<T,double>::value,
mat<T,4,4>>::type inverse(mat<T,4,4> m)
{
    // note: basically copy and pasted from glm.

    T Coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
    T Coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
    T Coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

    T Coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
    T Coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
    T Coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

    T Coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
    T Coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
    T Coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

    T Coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
    T Coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
    T Coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

    T Coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
    T Coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
    T Coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

    T Coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
    T Coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
    T Coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

    genType_base<T,4> Fac0(Coef00, Coef00, Coef02, Coef03);
    genType_base<T,4> Fac1(Coef04, Coef04, Coef06, Coef07);
    genType_base<T,4> Fac2(Coef08, Coef08, Coef10, Coef11);
    genType_base<T,4> Fac3(Coef12, Coef12, Coef14, Coef15);
    genType_base<T,4> Fac4(Coef16, Coef16, Coef18, Coef19);
    genType_base<T,4> Fac5(Coef20, Coef20, Coef22, Coef23);

    genType_base<T,4> Vec0(m[1][0], m[0][0], m[0][0], m[0][0]);
    genType_base<T,4> Vec1(m[1][1], m[0][1], m[0][1], m[0][1]);
    genType_base<T,4> Vec2(m[1][2], m[0][2], m[0][2], m[0][2]);
    genType_base<T,4> Vec3(m[1][3], m[0][3], m[0][3], m[0][3]);

    genType_base<T,4> Inv0(Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2);
    genType_base<T,4> Inv1(Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4);
    genType_base<T,4> Inv2(Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5);
    genType_base<T,4> Inv3(Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5);

    genType_base<T,4> SignA(+1, -1, +1, -1);
    genType_base<T,4> SignB(-1, +1, -1, +1);
    mat<T,4,4> Inverse(Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB);

    genType_base<T,4> Row0(Inverse[0][0], Inverse[1][0], Inverse[2][0], Inverse[3][0]);

    genType_base<T,4> Dot0(m[0] * Row0);
    T Dot1 = (Dot0.x + Dot0.y) + (Dot0.z + Dot0.w);

    if (std::abs(Dot1) <= std::numeric_limits<T>::epsilon())
    {
        throw std::logic_error("Matrix inversion failed");
    }

    T OneOverDeterminant = static_cast<T>(1) / Dot1;

    return Inverse * OneOverDeterminant;
}

template<class T>
typename std::enable_if<std::is_same<T,float>::value || std::is_same<T,double>::value,
mat<T,4,4>>::type Translate(T x, T y, T z)
{
    return {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { x, y, z, 1 }
    };
}

template<class T>
typename std::enable_if<std::is_same<T,float>::value || std::is_same<T,double>::value,
mat<T,4,4>>::type LookAt(genType_base<T,3> eye, genType_base<T,3> center, genType_base<T,3> up)
{
    auto f = normalize(center - eye);
    auto u = normalize(up);
    auto s = normalize(cross(f,u));
    u = cross(s,f);

    return {
        { s.x, u.x, -f.x, 0 },
        { s.y, u.y, -f.y, 0 },
        { s.z, u.z, -f.z, 0 },
        { -dot(s,eye), -dot(u,eye), dot(f,eye), 1 }
    };
}

template<class T>
typename std::enable_if<std::is_same<T,float>::value || std::is_same<T,double>::value,
mat<T,4,4>>::type Perspective(T fovy, T aspect, T zNear, T zFar)
{
    auto f = 1 / std::tan(fovy / 2);
    return {
        { f / aspect, 0, 0, 0 },
        { 0, f, 0, 0 },
        { 0, 0, (zFar + zNear) / (zNear - zFar), -1 },
        { 0, 0, (2 * zFar * zNear) / (zNear - zFar), 0 }
    };
}

template<class T>
typename std::enable_if<std::is_same<T,float>::value || std::is_same<T,double>::value,
genType_base<T,3>>::type UnProject(genType_base<T,3> windowCoordinate,
                      mat<T,4,4> modelView, mat<T,4,4> projection,
                      ivec4 viewport)
{
    mat<T,4,4> mvp = projection * modelView;

    mat<T,4,4> pmv = inverse(mvp);

    genType_base<T,4> normalizedCoordinates = {
        (windowCoordinate.x - (float) viewport[0]) / (float) viewport[2] * 2 - 1,
        (windowCoordinate.y - (float) viewport[1]) / (float) viewport[3] * 2 - 1,
        2 * windowCoordinate.z - 1,
        1
    };

    genType_base<T,4> perspected = pmv * normalizedCoordinates;

    if (std::abs(perspected[3]) <= std::numeric_limits<T>::epsilon())
    {
        throw std::logic_error("Can't unproject, no perspective component");
    }

    perspected[3] = 1 / perspected[3];

    return {
        perspected[0] * perspected[3],
        perspected[1] * perspected[3],
        perspected[2] * perspected[3]
    };
}

using mat2x2 = mat<float,2,2>;
using mat2x3 = mat<float,2,3>;
using mat2x4 = mat<float,2,4>;
using mat2 = mat2x2;

using mat3x2 = mat<float,3,2>;
using mat3x3 = mat<float,3,3>;
using mat3x4 = mat<float,3,4>;
using mat3 = mat3x3;

using mat4x2 = mat<float,4,2>;
using mat4x3 = mat<float,4,3>;
using mat4x4 = mat<float,4,4>;
using mat4 = mat4x4;

using dmat2x2 = mat<double,2,2>;
using dmat2x3 = mat<double,2,3>;
using dmat2x4 = mat<double,2,4>;
using dmat2 = mat2x2;

using dmat3x2 = mat<double,3,2>;
using dmat3x3 = mat<double,3,3>;
using dmat3x4 = mat<double,3,4>;
using dmat3 = mat3x3;

using dmat4x2 = mat<double,4,2>;
using dmat4x3 = mat<double,4,3>;
using dmat4x4 = mat<double,4,4>;
using dmat4 = mat4x4;

} // end namespace ng

#endif // NG_LINEARALGEBRA_HPP
