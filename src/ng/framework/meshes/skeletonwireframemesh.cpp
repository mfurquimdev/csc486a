#include "ng/framework/meshes/skeletonwireframemesh.hpp"

#include "ng/framework/models/skeletalmodel.hpp"

#include "ng/engine/math/linearalgebra.hpp"

#include "ng/engine/util/debug.hpp"

namespace ng
{

class SkeletonWireframeMesh::Vertex
{
public:
    vec3 Position;
    vec<std::uint8_t,4> Color;
};

SkeletonWireframeMesh::SkeletonWireframeMesh(
        std::shared_ptr<immutable<Skeleton>> skeleton,
        std::shared_ptr<immutable<SkinningMatrixPalette>> palette)
    : mSkeleton(std::move(skeleton))
    , mPalette(std::move(palette))
{ }


VertexFormat SkeletonWireframeMesh::GetVertexFormat() const
{
    VertexFormat fmt;

    fmt.PrimitiveType = PrimitiveType::Lines;

    fmt.Position = VertexAttribute(
                        3,
                        ArithmeticType::Float,
                        false,
                        sizeof(SkeletonWireframeMesh::Vertex),
                        offsetof(SkeletonWireframeMesh::Vertex,Position));

    fmt.Color = VertexAttribute(
                        4,
                        ArithmeticType::UInt8,
                        true,
                        sizeof(SkeletonWireframeMesh::Vertex),
                        offsetof(SkeletonWireframeMesh::Vertex,Color));

    return fmt;
}

std::size_t SkeletonWireframeMesh::GetMaxVertexBufferSize() const
{
    std::size_t numJoints = mSkeleton->get().Joints.size();
    if (numJoints > 0)
    {
        return (numJoints - 1) * 2 * sizeof(SkeletonWireframeMesh::Vertex);
    }
    else
    {
        return 0;
    }
}

std::size_t SkeletonWireframeMesh::GetMaxIndexBufferSize() const
{
    return 0;
}

std::size_t SkeletonWireframeMesh::WriteVertices(void* buffer) const
{
    std::size_t numJoints = mSkeleton->get().Joints.size();

    if (buffer != nullptr)
    {
        SkeletonWireframeMesh::Vertex* vertexBuffer =
            static_cast<SkeletonWireframeMesh::Vertex*>(buffer);

        std::size_t bufferOffset = 0;

        for (std::size_t j = 0; j < numJoints; j++)
        {
            const SkeletonJoint& joint = mSkeleton->get().Joints[j];

            if (joint.ParentIndex == SkeletonJoint::RootJointIndex)
            {
                continue;
            }

            const SkeletonJoint& parentJoint =
                mSkeleton->get().Joints[joint.ParentIndex];

            const mat4& mySkinningMatrix =
                mPalette->get().SkinningMatrices[j];

            const mat4& parentSkinningMatrix =
                mPalette->get().SkinningMatrices[joint.ParentIndex];

            vec4 myPos = mySkinningMatrix
                       * inverse(joint.InverseBindPose)
                       * vec4();

            vec4 parentPos = parentSkinningMatrix
                           * inverse(parentJoint.InverseBindPose)
                           * vec4();

            vertexBuffer[bufferOffset] =
                SkeletonWireframeMesh::Vertex{
                    vec3(myPos), vec<std::uint8_t,4>(0,255,0,255)
                };

            vertexBuffer[bufferOffset + 1] =
                SkeletonWireframeMesh::Vertex{
                    vec3(parentPos), vec<std::uint8_t,4>(255,0,0,255)
                };

            bufferOffset += 2;
        }
    }

    return numJoints > 0 ? (numJoints - 1) * 2 : 0;
}

std::size_t SkeletonWireframeMesh::WriteIndices(void*) const
{
    return 0;
}

} // end namespace ng
