#include "ng/framework/renderobjects/cubemesh.hpp"

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
    constexpr int SizeOfVertex = sizeof(vec3);
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
        { a, d, c }, { c, b, a }, // bottom
        { c, d, e }, { e, h, c }, // front
        { a, f, e }, { e, d, a }, // left
        { b, g, f }, { f, a, b }, // back
        { c, h, g }, { g, b, c }, // right
        { h, e, f }, { f, g, h }  // top
    };

    std::memcpy(buffer, vertexData, sizeof(vertexData));

    return 36;
}

std::size_t CubeMesh::WriteIndices(void*) const
{
    return 0;
}

} // end namespace ng
