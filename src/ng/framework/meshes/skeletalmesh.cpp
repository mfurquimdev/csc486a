#include "ng/framework/meshes/skeletalmesh.hpp"
#include "ng/framework/models/skeletalmodel.hpp"

#include "ng/engine/util/debug.hpp"

#include <cstring>

namespace ng
{

SkeletalMesh::SkeletalMesh(
        std::shared_ptr<IMesh> bindPoseMesh,
        std::shared_ptr<ImmutableSkinningMatrixPalette> skinningPalette)
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
                mSkinningPalette->GetPalette().SkinningMatrices;

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

            if (baseFmt.Position.Enabled)
            {
                char* pPosition = baseVertices.get()
                                + baseFmt.Position.Offset
                                + baseFmt.Position.Stride * i;

                float* pfPosition = reinterpret_cast<float*>(pPosition);

                vec4 bindPose4;
                std::memcpy(&bindPose4[0], pfPosition,
                          clampedPositionCardinality
                        * SizeOfArithmeticType(baseFmt.Position.Type));

                vec4 result(0);
                vec4 weights(
                        pJointWeights[0],
                        pJointWeights[1],
                        pJointWeights[2],
                        1.0f - pJointWeights[0]
                             - pJointWeights[1]
                             - pJointWeights[2]);

                //DebugPrintf("Weights: %f %f %f %f\n", weights[0], weights[1], weights[2], weights[3]);

                for (std::size_t j = 0; j < numJointsPerVertex; j++)
                {
                    std::uint8_t jointIndex = pJointIndices[j];
                    result += weights[j]
                            *  (skinningMatrices.at(jointIndex) * bindPose4);
                }

                std::memcpy(pfPosition, &result[0],
                          clampedPositionCardinality
                        * SizeOfArithmeticType(baseFmt.Position.Type));
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
