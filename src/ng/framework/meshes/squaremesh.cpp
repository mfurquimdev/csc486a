#include "ng/framework/meshes/squaremesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"

#include <cstring>

namespace ng
{

SquareMesh::SquareMesh(float sideLength)
    : mSideLength(sideLength)
{ }

VertexFormat SquareMesh::GetVertexFormat() const
{
    VertexFormat fmt;
    fmt.Position = VertexAttribute(2, ArithmeticType::Float, false, 0, 0);
    return fmt;
}

std::size_t SquareMesh::GetMaxVertexBufferSize() const
{
    constexpr int VerticesPerTriangle = 3;
    constexpr int NumTriangles = 2;
    constexpr std::size_t SizeOfVertex = sizeof(vec2);
    return VerticesPerTriangle * NumTriangles * SizeOfVertex;
}

std::size_t SquareMesh::GetMaxElementBufferSize() const
{
    return 0;
}

std::size_t SquareMesh::WriteVertices(void* buffer) const
{
    struct SquareVertex
    {
        vec2 Position;
    };

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

    const SquareVertex vertexData[][3] = {
        { a, b, c }, { c, d, a }
    };

    std::memcpy(buffer, vertexData, sizeof(vertexData));

    return 6;
}

std::size_t SquareMesh::WriteIndices(void*) const
{
    return 0;
}

} // end namespace ng
