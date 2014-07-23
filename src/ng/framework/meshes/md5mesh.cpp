#include "ng/framework/meshes/md5mesh.hpp"

namespace ng
{

class MD5MeshVertex
{
public:
    vec3 Position;
    vec2 Texcoord;
    vec3 Normal;
    vec<std::uint8_t,4> JointIndices;
    vec3 JointWeights;
};

MD5Mesh::MD5Mesh(MD5Model model)
    : mModel(std::move(model))
{ }

VertexFormat MD5Mesh::GetVertexFormat() const
{
    VertexFormat fmt;

    fmt.Position = VertexAttribute(
                3, ArithmeticType::Float, false,
                sizeof(MD5MeshVertex),
                offsetof(MD5MeshVertex,Position));

    fmt.TexCoord0 = VertexAttribute(
                2, ArithmeticType::Float, false,
                sizeof(MD5MeshVertex),
                offsetof(MD5MeshVertex,Texcoord));

    fmt.Normal = VertexAttribute(
                3, ArithmeticType::Float, false,
                sizeof(MD5MeshVertex),
                offsetof(MD5MeshVertex,Normal));

    fmt.JointIndices = VertexAttribute(
                4, ArithmeticType::UInt8, false,
                sizeof(MD5MeshVertex),
                offsetof(MD5MeshVertex,JointIndices));

    fmt.JointWeights = VertexAttribute(
                3, ArithmeticType::Float, false,
                sizeof(MD5MeshVertex),
                offsetof(MD5MeshVertex,JointWeights));

    fmt.IsIndexed = true;
    fmt.IndexType = ArithmeticType::UInt32;
    fmt.IndexOffset = 0;

    return fmt;
}

std::size_t MD5Mesh::GetMaxVertexBufferSize() const
{
    std::size_t size = 0;

    for (const MD5MeshData& mesh : mModel.Meshes)
    {
        std::size_t numTriangles = mesh.Triangles.size();
        size += numTriangles * 3 * sizeof(MD5MeshVertex);
    }

    return size;
}

std::size_t MD5Mesh::GetMaxIndexBufferSize() const
{
    std::size_t numTriangles = 0;

    for (const MD5MeshData& mesh : mModel.Meshes)
    {
        numTriangles += mesh.Triangles.size();
    }

    VertexFormat fmt = GetVertexFormat();

    return SizeOfArithmeticType(fmt.IndexType) * numTriangles * 3;
}

std::size_t MD5Mesh::WriteVertices(void* buffer) const
{
    MD5MeshVertex* vbuffer = static_cast<MD5MeshVertex*>(buffer);

    std::size_t vertexCount =
        std::accumulate(mModel.Meshes.begin(),
                        mModel.Meshes.end(),
                        0,
                        [](std::size_t count, const MD5MeshData& mesh) {
                            return count + mesh.Triangles.size() * 3;
                        });

    if (buffer != nullptr)
    {
        std::size_t index = 0;

        for (const MD5MeshData& mesh : mModel.Meshes)
        {
            for (std::size_t tri = 0;
                 tri < mesh.Triangles.size();
                 tri++, index += 3)
            {
                for (int i = 0; i < 3; i++)
                {
                    int vertexIndex = mesh.Triangles[tri].VertexIndices[i];
                    const MD5Vertex& md5vertex = mesh.Vertices[vertexIndex];

                    if (md5vertex.WeightCount > 4)
                    {
                        throw std::runtime_error(
                            "MD5Mesh only supports max 4 joints per vertex");
                    }

                    vec3 position;

                    for (int j = 0; j < md5vertex.WeightCount; j++)
                    {
                        const MD5Weight& md5weight =
                            mesh.Weights[md5vertex.StartWeight + j];
                        const MD5Joint& md5joint =
                            mModel.BindPoseJoints[md5weight.JointIndex];

                        vec3 wv;

                        // TODO: replace with my code
//                        Quat_rotatePoint(
//                            md5joint.Orientation,
//                            md5weight.WeightPosition,
//                            wv);

                        for (int component = 0; component < 3; component++)
                        {
                            position[component] =
                                (md5joint.Position[component] + wv[component])
                              * md5weight.WeightBias;
                        }
                    }

                    vec2 texcoord(
                        md5vertex.Texcoords[0],
                        1.0f - md5vertex.Texcoords[1]);


                    vec3 normal;
                    // TODO: compute normal

                    vec<std::uint8_t,4> jointIndices(0);
                    for (int j = 0; j < md5vertex.WeightCount; j++)
                    {
                        // should probably get the 4 most heavily weighted joints instead
                        jointIndices[j] = md5vertex.StartWeight + j;
                    }

                    vec3 jointWeights(0.0f);
                    for (int j = 0; j < 3 && j < md5vertex.WeightCount; j++)
                    {
                        jointWeights[j] =
                            mesh.Weights[md5vertex.StartWeight + j].WeightBias;
                    }

                    MD5MeshVertex& vertex = vbuffer[index];
                    vertex.Position = position;
                    vertex.Texcoord = texcoord;
                    vertex.Normal = normal;
                    vertex.JointIndices = jointIndices;
                    vertex.JointWeights = jointWeights;
                }
            }
        }
    }

    return vertexCount;
}

std::size_t MD5Mesh::WriteIndices(void* buffer) const
{
    std::uint32_t* ubuffer = static_cast<std::uint32_t*>(buffer);

    std::size_t index = 0;

    for (const MD5MeshData& mesh : mModel.Meshes)
    {
        for (std::size_t tri = 0; tri < mesh.Triangles.size(); tri++, index += 3)
        {
            std::copy(&mesh.Triangles[tri].VertexIndices[0],
                      &mesh.Triangles[tri].VertexIndices[0] + 3,
                      &ubuffer[index]);
        }
    }

    return index;
}

} // end namespace ng
