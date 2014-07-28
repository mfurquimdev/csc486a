#include "ng/framework/meshes/skeletalmesh.hpp"
#include "ng/framework/models/skeletalmodel.hpp"

#include "ng/engine/util/debug.hpp"

#include <cstring>

namespace ng
{

SkeletalMesh::SkeletalMesh(
        std::shared_ptr<IMesh> bindPoseMesh,
        std::shared_ptr<immutable<SkinningMatrixPalette>> skinningPalette)
    : mBindPoseMesh(std::move(bindPoseMesh))
    , mSkinningPalette(std::move(skinningPalette))
{
    if (mBindPoseMesh == nullptr)
    {
        throw std::logic_error("Cannot use null bind pose");
    }

    if (mSkinningPalette == nullptr)
    {
        throw std::logic_error("Cannot use null skinning palette");
    }

    VertexFormat baseFmt = mBindPoseMesh->GetVertexFormat();

    if (baseFmt.JointIndices.Enabled == false)
    {
        throw std::logic_error("SkeletalMesh requires JointIndices attribute");
    }

    if (baseFmt.JointIndices.Type != ArithmeticType::UInt8)
    {
        throw std::logic_error(
                    "SkeletalMesh requires JointIndices to be Uint8");
    }

    if (baseFmt.JointIndices.Cardinality != 4)
    {
        throw std::logic_error(
                    "SkeletalMesh requires JointIndices' cardinality to be 4");
    }

    if (baseFmt.JointWeights.Enabled == false)
    {
        throw std::logic_error("SkeletalMesh requires JointWeights attribute");
    }

    if (baseFmt.JointWeights.Type != ArithmeticType::Float)
    {
        throw std::logic_error(
                    "SkeletalMesh requires JointWeights to be Float");
    }

    if (baseFmt.JointWeights.Cardinality != 3)
    {
        throw std::logic_error(
                    "SkeletalMesh requires JointWeights' cardinality to be 3");
    }

    if (baseFmt.Position.Type != ArithmeticType::Float)
    {
        throw std::logic_error("SkeletalMesh requires Positions to be Float");
    }

    if (baseFmt.Position.Cardinality > 4)
    {
        throw std::logic_error(
                    "SkeletalMesh Positions' cardinality to be <= 4");
    }
}

VertexFormat SkeletalMesh::GetVertexFormat() const
{
    return mBindPoseMesh->GetVertexFormat();
}

std::size_t SkeletalMesh::GetMaxVertexBufferSize() const
{
    return mBindPoseMesh->GetMaxVertexBufferSize();
}

std::size_t SkeletalMesh::GetMaxIndexBufferSize() const
{
    return mBindPoseMesh->GetMaxIndexBufferSize();
}

std::size_t SkeletalMesh::WriteVertices(void* buffer) const
{
    std::size_t numBaseVertices = 0;
    std::size_t maxBaseVertexBufferSize =
            mBindPoseMesh->GetMaxVertexBufferSize();
    std::unique_ptr<char[]> baseVertices;

    if (maxBaseVertexBufferSize > 0)
    {
        if (buffer != nullptr)
        {
            baseVertices.reset(new char[maxBaseVertexBufferSize]);
        }

        numBaseVertices = mBindPoseMesh->WriteVertices(baseVertices.get());
    }

    if (buffer != nullptr)
    {
        VertexFormat baseFmt = mBindPoseMesh->GetVertexFormat();

        const unsigned int clampedPositionCardinality =
                std::min(4u, baseFmt.Position.Cardinality);

        const unsigned int numJointsPerVertex = 4;

        const std::vector<mat4>& skinningMatrices =
                mSkinningPalette->get().SkinningMatrices;

        std::vector<mat3> normalPalette;
        normalPalette.reserve(skinningMatrices.size());
        for (std::size_t j = 0; j < skinningMatrices.size(); j++)
        {
            normalPalette.push_back(
                mat3(transpose(inverse(skinningMatrices[j]))));
        }

        for (std::size_t i = 0; i < numBaseVertices; i++)
        {
            const std::uint8_t* pJointIndices =
                    reinterpret_cast<const std::uint8_t*>(
                        baseVertices.get()
                        + baseFmt.JointIndices.Offset
                        + baseFmt.JointIndices.Stride * i);

            const float* pJointWeights =
                    reinterpret_cast<const float*>(
                        baseVertices.get()
                        + baseFmt.JointWeights.Offset
                        + baseFmt.JointWeights.Stride * i);

            // compute the weight of the 4th joint using the first 3.
            vec4 weights(
                    pJointWeights[0],
                    pJointWeights[1],
                    pJointWeights[2],
                    1.0f - pJointWeights[0]
                         - pJointWeights[1]
                         - pJointWeights[2]);

            if (baseFmt.Position.Enabled)
            {
                float* pPosition = reinterpret_cast<float*>(
                                  baseVertices.get()
                                + baseFmt.Position.Offset
                                + baseFmt.Position.Stride * i);

                // get the position of the vertex in the bind pose mesh
                vec4 bindPosePosition;
                std::memcpy(&bindPosePosition[0], pPosition,
                          clampedPositionCardinality
                        * SizeOfArithmeticType(baseFmt.Position.Type));

                // accumulate the influence of the weights
                vec4 result(0);
                for (std::size_t jointRelativeIndex = 0;
                     jointRelativeIndex < numJointsPerVertex;
                     jointRelativeIndex++)
                {
                    std::uint8_t jointIndex = pJointIndices[jointRelativeIndex];
                    mat4 skinningMatrix = skinningMatrices.at(jointIndex);
                    result += weights[jointRelativeIndex]
                            * skinningMatrix * bindPosePosition;
                }

                // copy the resulting skinned position back into the mesh data
                std::memcpy(pPosition, &result[0],
                          clampedPositionCardinality
                        * SizeOfArithmeticType(baseFmt.Position.Type));
            }

            if (baseFmt.Normal.Enabled)
            {
                float* pNormal= reinterpret_cast<float*>(
                                  baseVertices.get()
                                + baseFmt.Normal.Offset
                                + baseFmt.Normal.Stride * i);

                // get the normal of the vertex in the bind pose mesh
                vec3 bindPoseNormal;
                std::memcpy(&bindPoseNormal[0], pNormal,
                          std::min(3u,baseFmt.Normal.Cardinality)
                        * SizeOfArithmeticType(baseFmt.Normal.Type));

                // accumulate the influence of the weights
                vec3 result(0);
                for (std::size_t jointRelativeIndex = 0;
                     jointRelativeIndex < numJointsPerVertex;
                     jointRelativeIndex++)
                {
                    std::uint8_t jointIndex = pJointIndices[jointRelativeIndex];
                    result += weights[jointRelativeIndex]
                            * normalPalette.at(jointIndex)
                            * bindPoseNormal;
                }

                result = normalize(result);

                // copy the resulting skinned normal back into the mesh data
                std::memcpy(pNormal, &result[0],
                          std::min(3u,baseFmt.Normal.Cardinality)
                        * SizeOfArithmeticType(baseFmt.Normal.Type));
            }
        }

        std::memcpy(buffer, baseVertices.get(), maxBaseVertexBufferSize);
    }

    return numBaseVertices;
}

std::size_t SkeletalMesh::WriteIndices(void* buffer) const
{
    return mBindPoseMesh->WriteIndices(buffer);
}

} // end namespace ng
