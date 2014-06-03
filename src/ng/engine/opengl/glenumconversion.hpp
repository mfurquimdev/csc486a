#ifndef NG_GLENUMCONVERSION_HPP
#define NG_GLENUMCONVERSION_HPP

#include "ng/engine/rendering/vertexformat.hpp"
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
         : throw std::logic_error("No define GL attribute index for this VertexAttributeName");
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
