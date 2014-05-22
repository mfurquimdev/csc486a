#ifndef NG_UNIFORM_HPP
#define NG_UNIFORM_HPP

#include "ng/engine/vec.hpp"
#include "ng/engine/mat.hpp"

#include <cstring>

namespace ng
{

enum class UniformType
{
    Float,
    Vec2,
    Vec3,
    Vec4,
    Mat3,
    Mat4
};

template<class T>
struct ToUniformType { };

template<>
struct ToUniformType<float>
{
    static const UniformType enum_value = UniformType::Float;
};

template<>
struct ToUniformType<vec2>
{
    static const UniformType enum_value = UniformType::Vec2;
};

template<>
struct ToUniformType<vec3>
{
    static const UniformType enum_value = UniformType::Vec3;
};

template<>
struct ToUniformType<vec4>
{
    static const UniformType enum_value = UniformType::Vec4;
};

template<>
struct ToUniformType<mat3>
{
    static const UniformType enum_value = UniformType::Mat3;
};

template<>
struct ToUniformType<mat4>
{
    static const UniformType enum_value = UniformType::Mat4;
};

struct UniformValue
{
    UniformType Type;

    union
    {
        float AsFloat;
        vec2  AsVec2;
        vec3  AsVec3;
        vec4  AsVec4;
        mat3  AsMat3;
        mat4  AsMat4;
    };

    UniformValue() = default;

    template<class T>
    UniformValue(T t)
        : Type(ToUniformType<T>::enum_value)
    {
        void* data = &AsFloat;
        std::memcpy(data, &t, sizeof(t));
    }
};

class IUniform
{
public:
    virtual ~IUniform() = default;

    virtual void SetValue(const UniformValue& value) = 0;
};

} // end namespace ng

#endif // NG_UNIFORM_HPP
