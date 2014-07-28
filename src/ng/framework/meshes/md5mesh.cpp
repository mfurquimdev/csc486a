#include "ng/framework/meshes/md5mesh.hpp"

#include "ng/engine/math/quaternion.hpp"

#include "ng/engine/util/debug.hpp"

#include <numeric>
#include <array>

namespace ng
{

class MD5Mesh::Vertex
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

    fmt.PrimitiveType = PrimitiveType::Triangles;

    fmt.Position = VertexAttribute(
                3, ArithmeticType::Float, false,
                sizeof(MD5Mesh::Vertex),
                offsetof(MD5Mesh::Vertex,Position));

    fmt.TexCoord0 = VertexAttribute(
                2, ArithmeticType::Float, false,
                sizeof(MD5Mesh::Vertex),
                offsetof(MD5Mesh::Vertex,Texcoord));

    fmt.Normal = VertexAttribute(
                3, ArithmeticType::Float, false,
                sizeof(MD5Mesh::Vertex),
                offsetof(MD5Mesh::Vertex,Normal));

    fmt.JointIndices = VertexAttribute(
                4, ArithmeticType::UInt8, false,
                sizeof(MD5Mesh::Vertex),
                offsetof(MD5Mesh::Vertex,JointIndices));

    fmt.JointWeights = VertexAttribute(
                3, ArithmeticType::Float, false,
                sizeof(MD5Mesh::Vertex),
                offsetof(MD5Mesh::Vertex,JointWeights));

    return fmt;
}

std::size_t MD5Mesh::GetMaxVertexBufferSize() const
{
    std::size_t size = 0;

    for (const MD5MeshData& mesh : mModel.Meshes)
    {
        std::size_t numTriangles = mesh.Triangles.size();
        size += numTriangles * 3 * sizeof(MD5Mesh::Vertex);
    }

    return size;
}

std::size_t MD5Mesh::GetMaxIndexBufferSize() const
{
    return 0;
}

std::size_t MD5Mesh::WriteVertices(void* buffer) const
{
    MD5Mesh::Vertex* vbuffer = static_cast<MD5Mesh::Vertex*>(buffer);

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
                std::array<vec3,3> positions;

                for (int posIndex = 0;
                     posIndex < 3;
                     posIndex++)
                {
                    int vertexIndex = mesh.Triangles[tri].VertexIndices[posIndex];
                    const MD5Vertex& md5vertex = mesh.Vertices[vertexIndex];

                    if (md5vertex.WeightCount > 4)
                    {
                        throw std::runtime_error(
                            "MD5Mesh only supports max 4 joints per vertex");
                    }

                    vec3& position = positions[posIndex];

                    // sum the influence of the weights
                    for (int weightRelativeIndex = 0;
                         weightRelativeIndex < md5vertex.WeightCount;
                         weightRelativeIndex++)
                    {
                        const MD5Weight& md5weight =
                            mesh.Weights.at(md5vertex.StartWeight
                                          + weightRelativeIndex);

                        const MD5Joint& md5joint =
                            mModel.BindPoseJoints.at(md5weight.JointIndex);

                        float quaternionW;
                        {
                            const vec3& q = md5joint.Orientation;
                            quaternionW = 1.0f - dot(q,q);
                            quaternionW  = quaternionW < 0.0f ?
                                        0.0f : - std::sqrt(quaternionW);
                        }

                        Quaternionf orientationQuaternion(
                                    Quaternionf::FromComponents(
                                        vec4(md5joint.Orientation, quaternionW)));

                        vec3 weightVector = vec3(rotate(
                                        orientationQuaternion,
                                        md5weight.WeightPosition).Components);

                        position += (md5joint.Position + weightVector)
                                  * md5weight.WeightBias;
                    }

                    // calculate GL texcoords
                    vec2 texcoord(
                        md5vertex.Texcoords[0],
                        1.0f - md5vertex.Texcoords[1]);

                    // get joint indices
                    vec<std::uint8_t,4> jointIndices(0);
                    for (int weightRelativeIndex = 0;
                         weightRelativeIndex < md5vertex.WeightCount;
                         weightRelativeIndex++)
                    {
                        const MD5Weight& md5weight =
                            mesh.Weights.at(md5vertex.StartWeight
                                          + weightRelativeIndex);

                        jointIndices[weightRelativeIndex] = md5weight.JointIndex;
                    }

                    // get joint weights
                    vec3 jointWeights(0.0f);

                    for (int weightRelativeIndex = 0;
                            weightRelativeIndex < 3 &&
                            weightRelativeIndex < md5vertex.WeightCount;
                         weightRelativeIndex++)
                    {
                        const MD5Weight& md5weight =
                            mesh.Weights.at(md5vertex.StartWeight
                                          + weightRelativeIndex);

                        jointWeights[weightRelativeIndex] = md5weight.WeightBias;
                    }

                    MD5Mesh::Vertex& vertex = vbuffer[index + posIndex];
                    vertex.Position = position;
                    vertex.Texcoord = texcoord;
                    vertex.JointIndices = jointIndices;
                    vertex.JointWeights = jointWeights;
                }

                // now that we have the positions, patch in normals
                vec3 tangent = positions[2] - positions[1];
                vec3 bitangent = positions[0] - positions[1];
                vec3 normal = normalize(cross(tangent, bitangent));

                for (int posIndex = 0; posIndex < 3; posIndex++)
                {
                    vbuffer[index + posIndex].Normal = normal;
                }
            }
        }
    }

    return vertexCount;
}

std::size_t MD5Mesh::WriteIndices(void*) const
{
    return 0;
}

} // end namespace ng
