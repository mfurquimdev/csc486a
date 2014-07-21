#include "ng/framework/meshes/objmesh.hpp"

#include "ng/engine/util/debug.hpp"

#include <cstring>

namespace ng
{

ObjMesh::ObjMesh(ObjModel shape)
    : mShape(std::move(shape))
{
    if (mShape.VerticesPerFace != 3)
    {
        throw std::runtime_error("Unhandled number of vertices per face "
                                 "(only handles 3)");
    }
}

static std::size_t GetVertexSize(const ObjModel& shape)
{
    std::size_t vertexSize = 0;

    if (shape.HasPositionIndices)
    {
        vertexSize += 4 * sizeof(float);
    }

    if (shape.HasTexcoordIndices)
    {
        vertexSize += 3 * sizeof(float);
    }

    if (shape.HasNormalIndices)
    {
        vertexSize += 3 * sizeof(float);
    }

    return vertexSize;
}

VertexFormat ObjMesh::GetVertexFormat() const
{
    VertexFormat fmt;

    std::size_t vertexSize = GetVertexSize(mShape);

    std::size_t offset = 0;
    if (mShape.HasPositionIndices)
    {
        fmt.Position = VertexAttribute(
                    4,
                    ArithmeticType::Float,
                    false,
                    vertexSize,
                    offset);
        offset += 4 * sizeof(float);
    }

    if (mShape.HasTexcoordIndices)
    {
        fmt.TexCoord0 = VertexAttribute(
                    3,
                    ArithmeticType::Float,
                    false,
                    vertexSize,
                    offset);
        offset += 3 * sizeof(float);
    }

    if (mShape.HasNormalIndices)
    {
        fmt.Normal = VertexAttribute(
                    3,
                    ArithmeticType::Float,
                    false,
                    vertexSize,
                    offset);
        offset += 3 * sizeof(float);
    }

    return fmt;
}

std::size_t ObjMesh::GetMaxVertexBufferSize() const
{
    int indicesPerVertex = int(mShape.HasPositionIndices)
                         + int(mShape.HasTexcoordIndices)
                         + int(mShape.HasNormalIndices);

    std::size_t vertexSize = GetVertexSize(mShape);
    std::size_t numFaces =
              mShape.Indices.size()
            / (indicesPerVertex * mShape.VerticesPerFace);
    std::size_t numVerticesTotal = numFaces * 3;
    return vertexSize * numVerticesTotal;
}

std::size_t ObjMesh::GetMaxIndexBufferSize() const
{
    // probably a viable optimization later on to index these meshes
    return 0;
}

std::size_t ObjMesh::WriteVertices(void* buffer) const
{
    int indicesPerVertex = int(mShape.HasPositionIndices)
                         + int(mShape.HasTexcoordIndices)
                         + int(mShape.HasNormalIndices);

    std::size_t numFaces =
              mShape.Indices.size()
            / (indicesPerVertex * mShape.VerticesPerFace);
    std::size_t numVerticesTotal = numFaces * 3;

    if (buffer != nullptr)
    {
        std::size_t vertexSize = GetVertexSize(mShape);
        std::size_t floatsPerVertex = vertexSize / sizeof(float);
        std::size_t floatsPerFace = 3 * floatsPerVertex;

        float* fbuffer = static_cast<float*>(buffer);

        for (std::size_t face = 0; face < numFaces; face++)
        {
            for (std::size_t v = 0; v < 3; v++)
            {
                std::size_t vertexDataIndex = face * floatsPerFace
                                            + v * floatsPerVertex;
                float* vertexData = &fbuffer[vertexDataIndex];
                const int* vertexIndices = mShape.Indices.data()
                                         + (face * 3 + v) * indicesPerVertex;

                if (mShape.HasPositionIndices)
                {
                    std::memcpy(vertexData,
                                &mShape.Positions[*vertexIndices - 1],
                                sizeof(vec4));
                    vertexIndices++;
                    vertexData += 4;
                }

                if (mShape.HasTexcoordIndices)
                {
                    std::memcpy(vertexData,
                                &mShape.Texcoords[*vertexIndices - 1],
                                sizeof(vec3));
                    vertexIndices++;
                    vertexData += 3;
                }

                if (mShape.HasNormalIndices)
                {
                    std::memcpy(vertexData,
                                &mShape.Normals[*vertexIndices - 1],
                                sizeof(vec3));
                    vertexIndices++;
                    vertexData += 3;
                }
            }
        }
    }

    return numVerticesTotal;
}

std::size_t ObjMesh::WriteIndices(void*) const
{
    return 0;
}

} // end namespace ng
