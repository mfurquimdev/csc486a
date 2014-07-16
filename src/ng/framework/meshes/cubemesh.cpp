#include "ng/framework/meshes/cubemesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"

#include <cstring>
#include <cstddef>

namespace ng
{

CubeMesh::CubeMesh(float sideLength)
    : mSideLength(sideLength)
{ }

VertexFormat CubeMesh::GetVertexFormat() const
{
    VertexFormat fmt;

    fmt.Position = VertexAttribute(
                3,
                ArithmeticType::Float,
                false,
                sizeof(CubeMesh::Vertex),
                offsetof(CubeMesh::Vertex, Position));

    fmt.Normal = VertexAttribute(
                3,
                ArithmeticType::Float,
                false,
                sizeof(CubeMesh::Vertex),
                offsetof(CubeMesh::Vertex, Normal));

    fmt.TexCoord0 = VertexAttribute(
                2,
                ArithmeticType::Float,
                false,
                sizeof(CubeMesh::Vertex),
                offsetof(CubeMesh::Vertex, Texcoord));

    return fmt;
}

std::size_t CubeMesh::GetMaxVertexBufferSize() const
{
    constexpr int NumFaces = 6;
    constexpr int TrianglesPerFace = 2;
    constexpr int VerticesPerTriangle = 3;
    constexpr std::size_t SizeOfVertex = sizeof(CubeMesh::Vertex);
    return NumFaces * TrianglesPerFace * VerticesPerTriangle * SizeOfVertex;
}

std::size_t CubeMesh::GetMaxIndexBufferSize() const
{
    return 0;
}

std::size_t CubeMesh::WriteVertices(void* buffer) const
{
    if (buffer)
    {
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

        // texcoords
        //
        //       * - - *
        //       |  +  |
        //       |  y  |
        // * - - * - - * - - * - - *
        // |  -  |  +  |  +  |  -  |
        // |  x  |  z  |  x  |  z  |
        // * - - * - - * - - * - - *
        //       |  -  |
        //       |  y  |
        //       * - - *

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

        const vec3    top( 0, 1, 0);
        const vec3 bottom( 0,-1, 0);
        const vec3   left(-1, 0, 0);
        const vec3  right( 1, 0, 0);
        const vec3  front( 0, 0, 1);
        const vec3   back( 0, 0,-1);

        const vec2 tx(
                    1.0f / 4.0f,
                    0.0f);
        const vec2 ty(
                    0.0f,
                    1.0f / 3.0f);

        // column, row. 0-indexed
        const vec2 t10 = tx * 1.0f + ty * 0.0f;
        const vec2 t20 = tx * 2.0f + ty * 0.0f;
        const vec2 t01 = tx * 0.0f + ty * 1.0f;
        const vec2 t11 = tx * 1.0f + ty * 1.0f;
        const vec2 t21 = tx * 2.0f + ty * 1.0f;
        const vec2 t31 = tx * 3.0f + ty * 1.0f;
        const vec2 t41 = tx * 4.0f + ty * 1.0f;
        const vec2 t02 = tx * 0.0f + ty * 2.0f;
        const vec2 t12 = tx * 1.0f + ty * 2.0f;
        const vec2 t22 = tx * 2.0f + ty * 2.0f;
        const vec2 t32 = tx * 3.0f + ty * 2.0f;
        const vec2 t42 = tx * 4.0f + ty * 2.0f;
        const vec2 t13 = tx * 1.0f + ty * 3.0f;
        const vec2 t23 = tx * 2.0f + ty * 3.0f;

        const CubeMesh::Vertex vertexData[][3] = {
            { { a, bottom, t10 }, { b, bottom, t20 }, { c, bottom, t21 } }, // bottom tri 1
            { { c, bottom, t21 }, { d, bottom, t11 }, { a, bottom, t10 } }, // bottom tri 2
            { { d, front,  t11 }, { c, front,  t21 }, { h, front,  t22 } }, // front tri 1
            { { h, front,  t22 }, { e, front,  t12 }, { d, front,  t11 } }, // front tri 2
            { { a, left,   t01 }, { d, left,   t11 }, { e, left,   t12 } }, // left tri 1
            { { e, left,   t12 }, { f, left,   t02 }, { a, left,   t01 } }, // left tri 2
            { { b, back,   t31 }, { a, back,   t41 }, { f, back,   t42 } }, // back tri 1
            { { f, back,   t42 }, { g, back,   t32 }, { b, back,   t31 } }, // back tri 2
            { { c, right,  t21 }, { b, right,  t31 }, { g, right,  t32 } }, // right tri 1
            { { g, right,  t32 }, { h, right,  t22 }, { c, right,  t21 } }, // right tri 2
            { { h, top,    t22 }, { g, top,    t23 }, { f, top,    t13 } }, // top tri 1
            { { f, top,    t13 }, { e, top,    t12 }, { h, top,    t22 } }  // top tri 2
        };

        std::memcpy(buffer, vertexData, sizeof(vertexData));
    }

    return 36;
}

std::size_t CubeMesh::WriteIndices(void*) const
{
    return 0;
}

} // end namespace ng
