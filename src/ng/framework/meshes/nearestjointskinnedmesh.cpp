#include "ng/framework/meshes/nearestjointskinnedmesh.hpp"
#include "ng/framework/models/skeletalmodel.hpp"

#include <cstring>

namespace ng
{

NearestJointSkinnedMesh::NearestJointSkinnedMesh(
        std::shared_ptr<IMesh> baseMesh,
        std::shared_ptr<ImmutableSkeleton> skeleton)
    : mBaseMesh(std::move(baseMesh))
    , mSkeleton(std::move(skeleton))
{
    if (mBaseMesh == nullptr)
    {
        throw std::logic_error("baseMesh cannot be nullptr");
    }

    if (mSkeleton == nullptr)
    {
        throw std::logic_error("skeleton cannot be nullptr");
    }

    if (mSkeleton->GetSkeleton().Joints.size() == 0)
    {
        throw std::logic_error("nearest joint skinning requires >= 1 joint");
    }

    VertexFormat baseFmt = mBaseMesh->GetVertexFormat();

    if (baseFmt.Position.Enabled == false)
    {
        throw std::logic_error("nearest joint skinning requires positions");
    }
}

VertexFormat NearestJointSkinnedMesh::GetVertexFormat() const
{
    std::size_t baseVertexBufferSize = mBaseMesh->GetMaxVertexBufferSize();

    VertexFormat fmt = mBaseMesh->GetVertexFormat();

    bool needToAddJointIndices = !(
               fmt.JointIndices.Enabled
            && fmt.JointIndices.Cardinality == 4
            && fmt.JointIndices.Type == ArithmeticType::UInt8);

    bool needToAddJointWeights = !(
                fmt.JointWeights.Enabled
             && fmt.JointWeights.Cardinality == 3
             && fmt.JointWeights.Type == ArithmeticType::Float);

    std::size_t stride = 0;

    if (needToAddJointIndices)
    {
        stride += sizeof(std::uint8_t) * 4;
    }

    if (needToAddJointWeights)
    {
        stride += sizeof(float) * 3;
    }

    std::size_t offset = 0;

    if (needToAddJointIndices)
    {
        fmt.JointIndices =
                VertexAttribute(
                    4, ArithmeticType::UInt8, false, stride,
                    baseVertexBufferSize + offset);
        offset += sizeof(std::uint8_t) * 4;
    }

    if (needToAddJointWeights)
    {
        fmt.JointWeights =
                VertexAttribute(
                    3, ArithmeticType::Float, false, stride,
                    baseVertexBufferSize + offset);
    }

    return fmt;
}

std::size_t NearestJointSkinnedMesh::GetMaxVertexBufferSize() const
{
    VertexFormat baseFmt = mBaseMesh->GetVertexFormat();

    bool needToAddJointIndices = !(
               baseFmt.JointIndices.Enabled
            && baseFmt.JointIndices.Cardinality == 4
            && baseFmt.JointIndices.Type == ArithmeticType::UInt8);

    bool needToAddJointWeights = !(
                baseFmt.JointWeights.Enabled
             && baseFmt.JointWeights.Cardinality == 3
             && baseFmt.JointWeights.Type == ArithmeticType::Float);

    std::size_t numBaseVertices = mBaseMesh->WriteVertices(nullptr);

    std::size_t baseMaxVertexBufferSize = mBaseMesh->GetMaxVertexBufferSize();

    return baseMaxVertexBufferSize +
            numBaseVertices * (
                  (needToAddJointIndices ? sizeof(std::uint8_t) * 4 : 0)
                + (needToAddJointWeights ? sizeof(float) * 3 : 0));
}

std::size_t NearestJointSkinnedMesh::GetMaxIndexBufferSize() const
{
    return mBaseMesh->GetMaxIndexBufferSize();
}

std::size_t NearestJointSkinnedMesh::WriteVertices(void* buffer) const
{
    std::size_t numBaseVertices = mBaseMesh->WriteVertices(nullptr);

    if (buffer != nullptr)
    {
        const Skeleton& skeleton = mSkeleton->GetSkeleton();

        std::size_t numJoints = skeleton.Joints.size();

        std::unique_ptr<vec3[]> jointPositions(new vec3[numJoints]);

        for (std::size_t j = 0; j < numJoints; j++)
        {
            jointPositions[j] = vec3(inverse(skeleton.Joints[j].InverseBindPose)
                                     * vec4());
        }

        std::size_t maxVertexBufferSize = GetMaxVertexBufferSize();
        VertexFormat fmt = GetVertexFormat();

        std::unique_ptr<char[]> vertexBuffer(
                    new char[maxVertexBufferSize]);

        mBaseMesh->WriteVertices(vertexBuffer.get());

        for (std::size_t v = 0; v < numBaseVertices; v++)
        {
            std::uint8_t closestVertices[4] = { 0, 0, 0, 0 };
            vec4 distanceSquaredToVertices(std::numeric_limits<float>::max());

            float furthestDistanceSquared = std::numeric_limits<float>::max();

            float* pPosition = reinterpret_cast<float*>(
                          vertexBuffer.get()
                        + fmt.Position.Offset
                        + fmt.Position.Stride * v);

            vec3 position;
            std::memcpy(&position[0], pPosition,
                    sizeof(float) * std::min(fmt.Position.Cardinality,3u));

            for (std::size_t j = 0; j < numJoints; j++)
            {
                float distanceSquared = dot(jointPositions[j], position);

                if (distanceSquared < furthestDistanceSquared)
                {
                    std::size_t furthestIndex = 0;
                    std::size_t numFurthestIndices = 0;
                    for (std::size_t k = 0; k < 4; k++)
                    {
                        if (distanceSquaredToVertices[k] ==
                            furthestDistanceSquared)
                        {
                            furthestIndex = k;
                            numFurthestIndices++;
                        }
                    }

                    if (numFurthestIndices == 1)
                    {
                        furthestDistanceSquared = distanceSquared;
                    }

                    distanceSquaredToVertices[furthestIndex] = distanceSquared;
                    closestVertices[furthestIndex] = j;
                }
            }

            char* indicesLocation =
                    vertexBuffer.get()
                  + fmt.JointIndices.Offset
                  + fmt.JointIndices.Stride * v;

            std::memcpy(indicesLocation, &closestVertices[0],
                    sizeof(std::uint8_t) * 4);

            float total = std::accumulate(
                        &distanceSquaredToVertices[0],
                        &distanceSquaredToVertices[3] + 1,
                        0.0f);

            vec4 weights = distanceSquaredToVertices / total;
            weights = vec4(1) - weights;
            float len = length(weights);
            if (len != 0.0f)
            {
                weights = weights / len;
            }

            char* weightsLocation =
                    vertexBuffer.get()
                  + fmt.JointWeights.Offset
                  + fmt.JointWeights.Stride * v;

            std::memcpy(weightsLocation, &weights,
                        sizeof(float) * 3);
        }

        std::memcpy(buffer, vertexBuffer.get(), maxVertexBufferSize);
    }

    return numBaseVertices;
}

std::size_t NearestJointSkinnedMesh::WriteIndices(void* buffer) const
{
    return mBaseMesh->WriteIndices(buffer);
}

} // end namespace ng
