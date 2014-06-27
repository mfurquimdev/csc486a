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

std::size_t CubeMesh::GetMaxNumVertices() const
{
    constexpr int NumFaces = 6;
    constexpr int TrianglesPerFace = 2;
    constexpr int VerticesPerTriangle = 3;
    return NumFaces * TrianglesPerFace * VerticesPerTriangle;
}

std::size_t CubeMesh::GetMaxNumIndices() const
{
    return 0;
}

std::size_t CubeMesh::WriteVertices(void* buffer) const
{
    struct CubeVertex
    {
        vec3 Position;
    };

    //   e----------f
    //  /|         /|
    // h----------g |
    // | |        | |
    // | |        | |
    // | a--------|-b
    // |/         |/
    // d----------c

    // positions
    const vec3 minExtent = vec3(-mSideLength / 2);
    const vec3 maxExtent = vec3( mSideLength / 2);

    const vec3 a = minExtent;
    const vec3 b = minExtent + vec3(mSideLength,0,0);
    const vec3 c = minExtent + vec3(0,0,mSideLength);
    const vec3 d = minExtent + vec3(mSideLength,0,mSideLength);
    const vec3 e = maxExtent - vec3(mSideLength,0,mSideLength);
    const vec3 f = maxExtent - vec3(0,0,mSideLength);
    const vec3 g = maxExtent - vec3(mSideLength,0,0);
    const vec3 h = maxExtent;

    const CubeVertex vertexData[][3] = {
        { a, d, c }, { c, b, a }, // bottom
        { c, d, h }, { h, g, c }, // front
        { a, e, h }, { h, d, a }, // left
        { b, f, e }, { e, a, b }, // back
        { c, g, f }, { f, b, c }, // right
        { h, e, f }, { f, g, h }  // top
    };

    std::memcpy(buffer, vertexData, sizeof(vertexData));

    return GetMaxNumVertices();
}

std::size_t CubeMesh::WriteIndices(void*) const
{
    return 0;
}

} // end namespace ng
