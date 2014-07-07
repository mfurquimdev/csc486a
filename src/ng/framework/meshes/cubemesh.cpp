#include "ng/framework/meshes/cubemesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"

#include <cstring>

namespace ng
{

CubeMesh::CubeMesh(float sideLength)
    : mSideLength(sideLength)
{ }

VertexFormat CubeMesh::GetVertexFormat() const
{
    VertexFormat fmt;
    fmt.Position = VertexAttribute(3, ArithmeticType::Float, false, 0, 0);
    return fmt;
}

std::size_t CubeMesh::GetMaxVertexBufferSize() const
{
    constexpr int NumFaces = 6;
    constexpr int TrianglesPerFace = 2;
    constexpr int VerticesPerTriangle = 3;
    constexpr std::size_t SizeOfVertex = sizeof(vec3);
    return NumFaces * TrianglesPerFace * VerticesPerTriangle * SizeOfVertex;
}

std::size_t CubeMesh::GetMaxElementBufferSize() const
{
    return 0;
}

std::size_t CubeMesh::WriteVertices(void* buffer) const
{
    struct CubeVertex
    {
        vec3 Position;
    };

    //   f----------g
    //  /|         /|
    // e----------h |
    // | |        | |
    // | |        | |
    // | a--------|-b
    // |/         |/
    // d----------c
    //
    //              -z
    //               ^
    //         +y   /
    //          ^  /
    //          | /
    //    -x <-- --> +x
    //         /|
    //        / v
    //       / -y
    //      v
    //     +z

    // positions
    const vec3 minExtent = vec3(-mSideLength / 2);
    const vec3 maxExtent = vec3( mSideLength / 2);

    const vec3 a = minExtent;
    const vec3 b = minExtent + vec3(mSideLength,0,0);
    const vec3 c = minExtent + vec3(mSideLength, 0, mSideLength);
    const vec3 d = minExtent + vec3(0,0,mSideLength);
    const vec3 e = maxExtent - vec3(mSideLength,0,0);
    const vec3 f = maxExtent - vec3(mSideLength,0,mSideLength);
    const vec3 g = maxExtent - vec3(0,0,mSideLength);
    const vec3 h = maxExtent;

    const CubeVertex vertexData[][3] = {
        { a, b, c }, { c, d, a }, // bottom
        { c, h, e }, { e, d, c }, // front
        { a, d, e }, { e, f, a }, // left
        { b, a, f }, { f, g, b }, // back
        { c, b, g }, { g, h, c }, // right
        { h, g, f }, { f, e, h }  // top
    };

    std::memcpy(buffer, vertexData, sizeof(vertexData));

    return 36;
}

std::size_t CubeMesh::WriteIndices(void*) const
{
    return 0;
}

} // end namespace ng