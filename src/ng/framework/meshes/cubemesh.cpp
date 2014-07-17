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

    fmt.IsIndexed = true;
    fmt.IndexType = ArithmeticType::UInt8;
    fmt.IndexOffset = 0;

    return fmt;
}

std::size_t CubeMesh::GetMaxVertexBufferSize() const
{
    constexpr int NumFaces = 6;
    constexpr int VerticesPerFace = 5;
    constexpr std::size_t SizeOfVertex = sizeof(CubeMesh::Vertex);
    return NumFaces * VerticesPerFace * SizeOfVertex;
}

std::size_t CubeMesh::GetMaxIndexBufferSize() const
{
    constexpr int NumFaces = 6;
    constexpr int TrianglesPerFace = 4;
    constexpr int IndicesPerTriangle = 3;
    constexpr std::size_t SizeOfIndex = sizeof(std::uint8_t);
    return NumFaces * TrianglesPerFace * IndicesPerTriangle * SizeOfIndex;
}

std::size_t CubeMesh::WriteVertices(void* buffer) const
{
    constexpr std::size_t numFaces = 6;
    constexpr std::size_t verticesPerFace = 5;

    if (buffer)
    {

        // positions
        // ================================
        //
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
        //
        // texcoords
        // ================================
        //
        // notation: tcr
        // c = column
        // r = row
        //
        //       t13---t23
        //        |  +  |
        //        |  y  |
        // t02---t12---t22---t32---t42
        //  |  -  |  +  |  +  |  -  |
        //  |  x  |  z  |  x  |  z  |
        // t01---t11---t21---t31---t41
        //        |  -  |
        //        |  y  |
        //       t10---t20

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

        const CubeMesh::Vertex zero;

        // six faces with 5 vertices each (bottom left, bottom right, top right, top left, center)
        // note the corner vertices are missing. They are patched in after by averaging the other vertices.
        CubeMesh::Vertex vertexData[numFaces][verticesPerFace] = {
            { { a, bottom, t10 }, { b, bottom, t20 }, { c, bottom, t21 }, { d, bottom, t11 }, zero }, // bottom
            { { d, front,  t11 }, { c, front,  t21 }, { h, front,  t22 }, { e, front,  t12 }, zero }, // front
            { { a, left,   t01 }, { d, left,   t11 }, { e, left,   t12 }, { f, left,   t02 }, zero }, // left
            { { b, back,   t31 }, { a, back,   t41 }, { f, back,   t42 }, { g, back,   t32 }, zero }, // back
            { { c, right,  t21 }, { b, right,  t31 }, { g, right,  t32 }, { h, right,  t22 }, zero }, // right
            { { e, top,    t12 }, { h, top,    t22 }, { g, top,    t23 }, { f, top,    t13 }, zero }, // top
        };

        // patch in the averaged centers
        for (std::size_t face = 0; face < numFaces; face++)
        {
            CubeMesh::Vertex& centerVertex = vertexData[face][4];

            for (std::size_t corner = 0; corner < 4; corner++)
            {
                CubeMesh::Vertex& cornerVertex = vertexData[face][corner];
                centerVertex.Position += cornerVertex.Position;
                centerVertex.Normal += cornerVertex.Normal;
                centerVertex.Texcoord += cornerVertex.Texcoord;
            }

            centerVertex.Position /= 4.0f;
            centerVertex.Normal /= 4.0f;
            centerVertex.Texcoord /= 4.0f;
        }

        std::memcpy(buffer, vertexData, sizeof(vertexData));
    }

    return numFaces * verticesPerFace;
}

std::size_t CubeMesh::WriteIndices(void* buffer) const
{
    constexpr std::size_t indicesPerFace = 12;
    constexpr std::size_t verticesPerFace = 5;
    constexpr std::size_t numFaces = 6;

    if (buffer != nullptr)
    {
        std::uint8_t* indices = static_cast<std::uint8_t*>(buffer);

        const std::uint8_t faceIndexPattern[indicesPerFace] = {
            0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4
        };

        for (std::size_t face = 0; face < numFaces; face++)
        {
            std::size_t bufferOffset = face * indicesPerFace;
            std::size_t indexOffset = face * verticesPerFace;

            std::uint8_t offsetIndices[indicesPerFace];
            std::memcpy(offsetIndices, faceIndexPattern, sizeof(faceIndexPattern));
            for (std::size_t i = 0; i < indicesPerFace; i++)
            {
                offsetIndices[i] += indexOffset;
            }

            std::memcpy(&indices[bufferOffset], offsetIndices, sizeof(offsetIndices));
        }

    }

    return indicesPerFace * numFaces;
}

} // end namespace ng
