#ifndef NG_VERTEXFORMAT_HPP
#define NG_VERTEXFORMAT_HPP

#include "ng/engine/arithmetictype.hpp"

#include <cstddef>

namespace ng
{

struct VertexAttribute
{
    int Cardinality;
    ArithmeticType Type;
    bool IsNormalized;
    std::ptrdiff_t Stride;
    std::size_t Offset;

    bool IsEnabled;

    VertexAttribute() = default;

    VertexAttribute(int cardinality,
                    ArithmeticType type,
                    bool isNormalized,
                    std::ptrdiff_t stride,
                    std::size_t offset,
                    bool isEnabled)
        : Cardinality(cardinality)
        , Type(type)
        , IsNormalized(isNormalized)
        , Stride(stride)
        , Offset(offset)
        , IsEnabled(isEnabled)
    { }
};

struct VertexFormat
{
    VertexAttribute Position;
    VertexAttribute Texcoord0;
    VertexAttribute Texcoord1;
    VertexAttribute Normal;

    bool IsIndexed;
    ArithmeticType IndexType;

    VertexFormat()
    {
        Position.IsEnabled = false;
        Texcoord0.IsEnabled = false;
        Texcoord1.IsEnabled = false;
        Normal.IsEnabled = false;
        IsIndexed = false;
    }
};

} // end namespace ng

#endif // NG_VERTEXFORMAT_HPP
