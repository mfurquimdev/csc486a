#ifndef NG_GLENUMCONVERSION_HPP
#define NG_GLENUMCONVERSION_HPP

#include "ng/engine/arithmetictype.hpp"
#include "ng/engine/primitivetype.hpp"

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

} // end namespace ng

#endif // NG_GLENUMCONVERSION_HPP
