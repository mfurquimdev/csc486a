#ifndef NG_GLENUMCONVERSION_HPP
#define NG_GLENUMCONVERSION_HPP

#include "ng/engine/primitivetype.hpp"

#include <GL/gl.h>

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
         : throw std::logic_error("No such PrimitiveType");
}

} // end namespace ng

#endif // NG_GLENUMCONVERSION_HPP
