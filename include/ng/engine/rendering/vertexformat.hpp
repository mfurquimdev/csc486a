#ifndef NG_VERTEXFORMAT_HPP
#define NG_VERTEXFORMAT_HPP

#include "ng/engine/util/arithmetictype.hpp"

#include <array>
#include <functional>

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
        : Enabled(true)
        , Cardinality(cardinality)
        , Type(type)
        , Normalized(normalized)
        , Stride(stride)
        , Offset(offset)
    { }
};

class VertexFormat
{
public:  
    VertexAttribute Position;  // usually 2-4 floats
    VertexAttribute Normal;    // usually 3 floats
    VertexAttribute TexCoord0; // usually 2-3 floats

    // for skeletal meshes
    VertexAttribute JointIndices; // usually 4 unsigned bytes
    VertexAttribute JointWeights; // usually 3 floats (4th is calculated)

    bool IsIndexed = false;
    ArithmeticType IndexType;
    std::size_t IndexOffset;
};

std::array<std::reference_wrapper<const VertexAttribute>,5>
    GetAttribArray(const VertexFormat& fmt);

std::array<std::reference_wrapper<VertexAttribute>,5>
    GetAttribArray(VertexFormat& fmt);

} // end namespace ng

#endif // NG_VERTEXFORMAT_HPP
