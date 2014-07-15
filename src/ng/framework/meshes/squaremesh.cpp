#include "ng/framework/meshes/squaremesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"

#include <cstring>
#include <cstddef>

namespace ng
{

SquareMesh::SquareMesh(float sideLength)
    : mSideLength(sideLength)
{ }

VertexFormat SquareMesh::GetVertexFormat() const
{
    VertexFormat fmt;

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

    return fmt;
}

std::size_t SquareMesh::GetMaxVertexBufferSize() const
{
    constexpr int VerticesPerTriangle = 3;
    constexpr int NumTriangles = 2;
    constexpr std::size_t SizeOfVertex = sizeof(SquareMesh::Vertex);
    return VerticesPerTriangle * NumTriangles * SizeOfVertex;
}

std::size_t SquareMesh::GetMaxIndexBufferSize() const
{
    return 0;
}

std::size_t SquareMesh::WriteVertices(void* buffer) const
{
    if (buffer)
    {
        // a ---- d
        // | \    |
        // |  \   |
        // |   \  |
        // |    \ |
        // b ---- c

        vec2 minExtent(-mSideLength/2);
        vec2 maxExtent(mSideLength/2);

        vec2 a = minExtent + vec2(0,mSideLength);
        vec2 b = minExtent;
        vec2 c = minExtent + vec2(mSideLength,0);
        vec2 d = maxExtent;

        vec2 ta(0.0f, 1.0f);
        vec2 tb(0.0f, 0.0f);
        vec2 tc(1.0f, 0.0f);
        vec2 td(1.0f, 1.0f);

        vec3 n = vec3(0,0,1);

        const SquareMesh::Vertex vertexData[][3] = {
            { { a, ta, n }, { b, tb, n }, { c, tc, n } }, // bottom left
            { { c, tc, n }, { d, td, n }, { a, ta, n } }  // top right
        };

        std::memcpy(buffer, vertexData, sizeof(vertexData));
    }

    return 6;
}

std::size_t SquareMesh::WriteIndices(void*) const
{
    return 0;
}

} // end namespace ng
