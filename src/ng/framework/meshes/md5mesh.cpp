#include "ng/framework/meshes/md5mesh.hpp"

namespace ng
{

MD5Mesh::MD5Mesh(MD5Model model)
    : mModel(std::move(model))
{ }

static const std::size_t MD5MeshVertexSize =
        3 * sizeof(float) // positions
      + 2 * sizeof(float) // texcoords
      + 3 * sizeof(float) // normals
      + 4 * sizeof(std::uint8_t) // joint indices
      + 4 * sizeof(float); // joint weights

VertexFormat MD5Mesh::GetVertexFormat() const
{
    VertexFormat fmt;

    std::size_t vertexSize = MD5MeshVertexSize;

    std::size_t offset = 0;

    fmt.Position = VertexAttribute(
                3, ArithmeticType::Float, false,
                vertexSize, offset);
    offset += 3 * sizeof(float);

    fmt.TexCoord0 = VertexAttribute(
                2, ArithmeticType::Float, false,
                vertexSize, offset);
    offset += 2 * sizeof(float);

    fmt.Normal = VertexAttribute(
                3, ArithmeticType::Float, false,
                vertexSize, offset);
    offset += 3 * sizeof(float);

    fmt.JointIndices = VertexAttribute(
                4, ArithmeticType::UInt8, false,
                vertexSize, offset);
    offset += 4 * sizeof(std::uint8_t);

    fmt.JointWeights = VertexAttribute(
                4, ArithmeticType::Float, false,
                vertexSize, offset);
    offset += 4 * sizeof(float);

    return fmt;
}

std::size_t MD5Mesh::GetMaxVertexBufferSize() const
{
    std::size_t size = 0;

    for (const MD5MeshData& mesh : mModel.Meshes)
    {
        std::size_t numTriangles = mesh.Triangles.size();
        size += numTriangles * 3 * MD5MeshVertexSize;
    }

    return size;
}

std::size_t MD5Mesh::GetMaxIndexBufferSize() const
{

}

std::size_t MD5Mesh::WriteVertices(void* buffer) const
{

}

std::size_t MD5Mesh::WriteIndices(void* buffer) const
{

}

} // end namespace ng
