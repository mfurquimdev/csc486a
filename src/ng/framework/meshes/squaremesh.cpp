#include "ng/framework/meshes/squaremesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"

#include <cstring>
#include <cstddef>

namespace ng
{

class SquareMesh::Vertex
{
public:
    vec2 Position;
    vec2 Texcoord;
    vec3 Normal;
};

SquareMesh::SquareMesh(float sideLength)
    : mSideLength(sideLength)
{ }

VertexFormat SquareMesh::GetVertexFormat() const
{
    VertexFormat fmt;

    fmt.PrimitiveType = PrimitiveType::Triangles;

    fmt.Position = VertexAttribute(
                2,
                ArithmeticType::Float,
                false,
                sizeof(SquareMesh::Vertex),
                offsetof(SquareMesh::Vertex, Position));

    fmt.TexCoord0 = VertexAttribute(
                2,
                ArithmeticType::Float,
                false,
                sizeof(SquareMesh::Vertex),
                offsetof(SquareMesh::Vertex, Texcoord));

    fmt.Normal = VertexAttribute(
                3,
                ArithmeticType::Float,
                false,
                sizeof(SquareMesh::Vertex),
                offsetof(SquareMesh::Vertex, Normal));

    fmt.IsIndexed = true;
    fmt.IndexType = ArithmeticType::UInt8;
    fmt.IndexOffset = 0;

    return fmt;
}

std::size_t SquareMesh::GetMaxVertexBufferSize() const
{
    constexpr int NumVertices = 5;
    constexpr std::size_t SizeOfVertex = sizeof(SquareMesh::Vertex);
    return NumVertices * SizeOfVertex;
}

std::size_t SquareMesh::GetMaxIndexBufferSize() const
{
    constexpr int NumTriangles = 4;
    constexpr int IndicesPerTriangle = 3;
    constexpr std::size_t SizeOfIndex = sizeof(std::uint8_t);
    return NumTriangles * IndicesPerTriangle * SizeOfIndex;
}

std::size_t SquareMesh::WriteVertices(void* buffer) const
{
    if (buffer)
    {
        // positions
        // d ---- c
        // |      |
        // |      |
        // |      |
        // |      |
        // a ---- b

        vec2 minExtent(-mSideLength/2);
        vec2 maxExtent(mSideLength/2);

        vec2 a = minExtent;
        vec2 b = minExtent + vec2(mSideLength,0);
        vec2 c = maxExtent;
        vec2 d = minExtent + vec2(0,mSideLength);

        vec2 ta(0.0f, 0.0f);
        vec2 tb(1.0f, 0.0f);
        vec2 tc(1.0f, 1.0f);
        vec2 td(0.0f, 1.0f);

        vec3 n = vec3(0,0,1);

        SquareMesh::Vertex zero;
        // 5 vertices  (bottom left, bottom right, top right, top left, center)
        SquareMesh::Vertex vertexData[5] = {
            { a, ta, n }, { b, tb, n }, { c, tc, n }, { d, td, n }, zero
        };

        // patch in the averaged center
        SquareMesh::Vertex& center = vertexData[4];
        for (std::size_t cornerIdx = 0; cornerIdx < 4; cornerIdx++)
        {
            SquareMesh::Vertex& corner = vertexData[cornerIdx];
            center.Position += corner.Position;
            center.Texcoord += corner.Texcoord;
            center.Normal += corner.Normal;
        }

        center.Position /= 4.0f;
        center.Texcoord /= 4.0f;
        center.Normal /= 4.0f;

        std::memcpy(buffer, vertexData, sizeof(vertexData));
    }

    return 6;
}

std::size_t SquareMesh::WriteIndices(void* buffer) const
{
    if (buffer != nullptr)
    {
        std::uint8_t* indices = static_cast<std::uint8_t*>(buffer);
        const std::uint8_t indexData[] = {
            0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4
        };
        std::memcpy(indices, indexData, sizeof(indexData));
    }

    return 12;
}

} // end namespace ng
