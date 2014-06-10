#ifndef NG_GLENUMCONVERSION_HPP
#define NG_GLENUMCONVERSION_HPP

#include "ng/engine/rendering/vertexformat.hpp"
#include "ng/engine/rendering/renderstate.hpp"
#include "ng/engine/util/arithmetictype.hpp"
#include "ng/engine/rendering/primitivetype.hpp"

#include <GL/gl.h>

#include <stdexcept>

namespace ng
{

constexpr GLenum ToGLPrimitiveType(PrimitiveType pt)
{
    return pt == PrimitiveType::Points ? GL_POINTS
         : pt == PrimitiveType::LineStrip ? GL_LINE_STRIP
         : pt == PrimitiveType::LineLoop ? GL_LINE_LOOP
         : pt == PrimitiveType::Lines ? GL_LINES
         : pt == PrimitiveType::TriangleStrip ? GL_TRIANGLE_STRIP
         : pt == PrimitiveType::TriangleFan ? GL_TRIANGLE_FAN
         : pt == PrimitiveType::Triangles ? GL_TRIANGLES
         : throw std::logic_error("No GL equivalent to this PrimitiveType");
}

constexpr PrimitiveType ToNGPrimitiveType(GLenum pt)
{
    return pt == GL_POINTS ? PrimitiveType::Points
         : pt == GL_LINE_STRIP ? PrimitiveType::LineStrip
         : pt == GL_LINE_LOOP ? PrimitiveType::LineLoop
         : pt == GL_LINES ? PrimitiveType::Lines
         : pt == GL_TRIANGLE_STRIP ? PrimitiveType::TriangleStrip
         : pt == GL_TRIANGLE_FAN ? PrimitiveType::TriangleFan
         : pt == GL_TRIANGLES ? PrimitiveType::Triangles
         : throw std::logic_error("No PrimitiveType equivalent to this GLenum primitive");
}

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

constexpr GLuint ToGLAttributeIndex(VertexAttributeName va)
{
    return va == VertexAttributeName::Position ? 0
         : va == VertexAttributeName::Texcoord0 ? 1
         : va == VertexAttributeName::Texcoord1 ? 2
         : va == VertexAttributeName::Normal ? 3
         : throw std::logic_error("No defined GL attribute index for this VertexAttributeName");
}

constexpr GLenum ToGLDepthFunc(DepthTestFunc d)
{
    return d == DepthTestFunc::Never ? GL_NEVER
         : d == DepthTestFunc::Less ? GL_LESS
         : d == DepthTestFunc::Equal ? GL_EQUAL
         : d == DepthTestFunc::LessOrEqual ? GL_LEQUAL
         : d == DepthTestFunc::Greater ? GL_GREATER
         : d == DepthTestFunc::NotEqual ? GL_NOTEQUAL
         : d == DepthTestFunc::GreaterOrEqual ? GL_GEQUAL
         : d == DepthTestFunc::Always ? GL_ALWAYS
         : throw std::logic_error("No GL equivalent for this DepthTestFunc");
}

constexpr GLenum ToGLBlendMode(BlendMode b)
{
    return b == BlendMode::Zero ? GL_ZERO
         : b == BlendMode::One ? GL_ONE
         : b == BlendMode::SourceColor ? GL_SRC_COLOR
         : b == BlendMode::OneMinusSourceColor ? GL_ONE_MINUS_SRC_COLOR
         : b == BlendMode::DestinationColor ? GL_DST_COLOR
         : b == BlendMode::OneMinusDestinationColor ? GL_ONE_MINUS_DST_COLOR
         : b == BlendMode::SourceAlpha ? GL_SRC_ALPHA
         : b == BlendMode::OneMinusSourceAlpha ? GL_ONE_MINUS_SRC_ALPHA
         : b == BlendMode::DestinationAlpha ? GL_DST_ALPHA
         : b == BlendMode::OneMinusDestinationAlpha ? GL_ONE_MINUS_DST_ALPHA
         : b == BlendMode::ConstantColor ? GL_CONSTANT_COLOR
         : b == BlendMode::OneMinusConstantColor ? GL_ONE_MINUS_CONSTANT_COLOR
         : b == BlendMode::ConstantAlpha ? GL_CONSTANT_ALPHA
         : b == BlendMode::OneMinusConstantAlpha ? GL_ONE_MINUS_CONSTANT_ALPHA
         : b == BlendMode::SourceAlphaSaturate ? GL_SRC_ALPHA_SATURATE
         : throw std::logic_error("No GL equivalent for this BlendMode");
}

constexpr const char* OpenGLErrorCodeToString(GLenum e)
{
    return e == GL_NO_ERROR ? "GL_NO_ERROR"
         : e == GL_INVALID_ENUM ? "GL_INVALID_ENUM"
         : e == GL_INVALID_VALUE ? "GL_INVALID_VALUE"
         : e == GL_INVALID_OPERATION ? "GL_INVALID_OPERATION"
         : e == GL_STACK_OVERFLOW ? "GL_STACK_OVERFLOW"
         : e == GL_STACK_UNDERFLOW ? "GL_STACK_UNDERFLOW"
         : e == GL_OUT_OF_MEMORY ? "GL_OUT_OF_MEMORY"
         : throw std::logic_error("No such GL error code");
}

} // end namespace ng

#endif // NG_GLENUMCONVERSION_HPP
