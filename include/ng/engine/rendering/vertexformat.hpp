#ifndef NG_VERTEXFORMAT_HPP
#define NG_VERTEXFORMAT_HPP

#include "ng/engine/util/arithmetictype.hpp"

#include <array>

namespace ng
{

class VertexAttribute
{
public:
    bool Enabled = false;

    unsigned int Cardinality;
    ArithmeticType Type;
    bool Normalized;
    std::ptrdiff_t Stride;
    std::size_t Offset;

    VertexAttribute() = default;

    VertexAttribute(unsigned int cardinality,
                    ArithmeticType type,
                    bool normalized,
                    std::ptrdiff_t stride,
                    std::size_t offset)
        : Cardinality(cardinality)
        , Type(type)
        , Normalized(normalized)
        , Stride(stride)
        , Offset(offset)
    { }
};

class VertexFormat
{
public:
    VertexAttribute Position;
    VertexAttribute Normal;
    std::array<VertexAttribute,8> TexCoords;

    bool IsIndexed = false;
    ArithmeticType IndexType;
    std::size_t IndexOffset;
};

} // end namespace ng

#endif // NG_VERTEXFORMAT_HPP
