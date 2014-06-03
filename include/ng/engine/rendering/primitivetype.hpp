#ifndef NG_PRIMITIVETYPE_HPP
#define NG_PRIMITIVETYPE_HPP

#include <stdexcept>

namespace ng
{

enum class PrimitiveType
{
    Points,
    LineStrip,
    LineLoop,
    Lines,
    TriangleStrip,
    TriangleFan,
    Triangles
};

constexpr const char* PrimitiveTypeToString(PrimitiveType pt)
{
    return pt == PrimitiveType::Points ? "Points"
         : pt == PrimitiveType::LineStrip ? "LineStrip"
         : pt == PrimitiveType::LineLoop ? "LineLoop"
         : pt == PrimitiveType::Lines ? "Lines"
         : pt == PrimitiveType::TriangleStrip ? "TriangleStrip"
         : pt == PrimitiveType::TriangleFan ? "TriangleFan"
         : pt == PrimitiveType::Triangles ? "Triangles"
         : throw std::logic_error("No such PrimitiveType");
}

} // end namespace ng

#endif // NG_PRIMITIVETYPE_HPP
