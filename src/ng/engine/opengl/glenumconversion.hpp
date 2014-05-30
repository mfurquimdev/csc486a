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

} // end namespace ng

#endif // NG_GLENUMCONVERSION_HPP
