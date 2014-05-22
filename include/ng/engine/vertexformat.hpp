#ifndef NG_VERTEXFORMAT_HPP
#define NG_VERTEXFORMAT_HPP

#include "ng/engine/arithmetictype.hpp"

#include <cstddef>
#include <map>

namespace ng
{

enum class VertexAttributeName
{
    Position,
    Texcoord0,
    Texcoord1,
    Normal
};

struct VertexAttribute
{
    int Cardinality;
    ArithmeticType Type;
    bool IsNormalized;
    std::ptrdiff_t Stride;
    std::size_t Offset;

    VertexAttribute() = default;

    VertexAttribute(
            int cardinality,
            ArithmeticType type,
            bool isNormalized,
            std::ptrdiff_t stride,
            std::size_t offset)
        : Cardinality(cardinality)
        , Type(type)
        , IsNormalized(isNormalized)
        , Stride(stride)
        , Offset(offset)
    { }
};

struct VertexFormat
{
    using AttributeMap = std::map<VertexAttributeName, VertexAttribute>;

    AttributeMap Attributes;

    bool IsIndexed = false;
    ArithmeticType IndexType;

    VertexFormat() = default;

    VertexFormat(AttributeMap attributes)
        : Attributes(std::move(attributes))
    { }

    VertexFormat(AttributeMap attributes, ArithmeticType indexType)
        : Attributes(std::move(attributes))
        , IsIndexed(true)
        , IndexType(indexType)
    { }
};

} // end namespace ng

#endif // NG_VERTEXFORMAT_HPP
