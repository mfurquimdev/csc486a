#ifndef NG_OPENGLENUMCONVERSION_HPP
#define NG_OPENGLENUMCONVERSION_HPP

#include "ng/engine/rendering/sampler.hpp"
#include "ng/engine/rendering/vertexformat.hpp"

#include "ng/engine/util/arithmetictype.hpp"

#include <GL/gl.h>

#include <stdexcept>

namespace ng
{

constexpr GLenum ToGLArithmeticType(ArithmeticType at)
{
    return at == ArithmeticType::Int8 ? GL_BYTE
         : at == ArithmeticType::Int16 ? GL_SHORT
         : at == ArithmeticType::Int32 ? GL_INT
         : at == ArithmeticType::UInt8 ? GL_UNSIGNED_BYTE
         : at == ArithmeticType::UInt16 ? GL_UNSIGNED_SHORT
         : at == ArithmeticType::UInt32 ? GL_UNSIGNED_INT
         : at == ArithmeticType::Float ? GL_FLOAT
         : at == ArithmeticType::Double ? GL_DOUBLE
         : throw std::logic_error("No GL equivalent to this ArithmeticType");
}

constexpr GLenum ToGLTextureFilter(TextureFilter f)
{
    return f == TextureFilter::Linear ? GL_LINEAR
         : f == TextureFilter::Nearest ? GL_NEAREST
         : throw std::logic_error("No GL equivalent to this TextureFilter");
}

constexpr GLenum ToGLTextureWrap(TextureWrap w)
{
    return w == TextureWrap::Repeat ? GL_REPEAT
         : w == TextureWrap::ClampToEdge ? GL_CLAMP_TO_EDGE
         : throw std::logic_error("No GL equivalent to this TextureWrap");
}

constexpr GLenum ToGLPrimitive(PrimitiveType p)
{
    return p == PrimitiveType::Triangles ? GL_TRIANGLES
         : p == PrimitiveType::Lines ? GL_LINES
         : throw std::logic_error("No GL equivalent to this PrimitiveType");
}

} // end namespace ng

#endif // NG_OPENGLENUMCONVERSION_HPP
