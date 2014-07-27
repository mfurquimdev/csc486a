#include "ng/framework/meshes/basismesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"

#include <cstring>

namespace ng
{

class BasisMesh::Vertex
{
public:
    vec3 Position;
    vec<std::uint8_t,4> Color;
};

VertexFormat BasisMesh::GetVertexFormat() const
{
    VertexFormat fmt;

    fmt.PrimitiveType = PrimitiveType::Lines;

    fmt.Position = VertexAttribute(
                        3,
                        ArithmeticType::Float,
                        false,
                        sizeof(BasisMesh::Vertex),
                        offsetof(BasisMesh::Vertex,Position));

    fmt.Color = VertexAttribute(
                        4,
                        ArithmeticType::UInt8,
                        true,
                        sizeof(BasisMesh::Vertex),
                        offsetof(BasisMesh::Vertex,Color));

    return fmt;
}

std::size_t BasisMesh::GetMaxVertexBufferSize() const
{
    return sizeof(BasisMesh::Vertex) * 6;
}

std::size_t BasisMesh::GetMaxIndexBufferSize() const
{
    return 0;
}

std::size_t BasisMesh::WriteVertices(void* buffer) const
{
    if (buffer != nullptr)
    {
        const BasisMesh::Vertex vertexData[] = {
            { { 0.0f, 0.0f, 0.0f }, { 255, 0, 0, 255 } },
            { { 1.0f, 0.0f, 0.0f }, { 255, 0, 0, 255 } },
            { { 0.0f, 0.0f, 0.0f }, { 0, 255, 0, 255 } },
            { { 0.0f, 1.0f, 0.0f }, { 0, 255, 0, 255 } },
            { { 0.0f, 0.0f, 0.0f }, { 0, 0, 255, 255 } },
            { { 0.0f, 0.0f, 1.0f }, { 0, 0, 255, 255 } },
        };

        std::memcpy(buffer, vertexData, sizeof(vertexData));
    }

    return 6;
}

std::size_t BasisMesh::WriteIndices(void*) const
{
    return 0;
}

} // end namespace ng
