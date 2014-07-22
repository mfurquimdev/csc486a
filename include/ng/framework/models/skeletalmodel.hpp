#ifndef NG_SKELETALMODEL_HPP
#define NG_SKELETALMODEL_HPP

#include "ng/engine/math/linearalgebra.hpp"
#include "ng/engine/math/quaternion.hpp"
#include "ng/engine/util/compilertraits.hpp"

#include <vector>

namespace ng
{

class SkeletonJoint
{
public:
    mat4 InverseBindPose;
    std::string JointName;

    static constexpr int RootJointIndex = -1;
    int ParentIndex;
};

class Skeleton
{
public:
    std::vector<SkeletonJoint> Joints;
};

class ImmutableSkeleton
{
    Skeleton mSkeleton;

public:
    ImmutableSkeleton(Skeleton skeleton)
        : mSkeleton(std::move(skeleton))
    { }

    const Skeleton& GetSkeleton() const
    {
        return mSkeleton;
    }
};

class MD5Model;

void SkeletonFromMD5Model(
        const MD5Model& model,
        Skeleton& skeleton);

class SkeletonJointPose
{
public:
    Quaternionf Rotation;
    vec3 Translation;
    vec3 Scale{1};

private:
    std::uint8_t Padding[8];
};

mat4 PoseToMat4(const SkeletonJointPose& pose);

void CalculateInverseBindPose(
        const SkeletonJointPose* bindPoseJointPoses,
        SkeletonJoint* joints,
        std::size_t numJoints);

void LocalPosesToGlobalPoses(
        const SkeletonJoint* joints,
        const SkeletonJointPose* localPoses,
        std::size_t numJoints,
        mat4* globalPoses);

class SkinningMatrixPalette
{
public:
    std::vector<mat4> SkinningMatrices;
};

class ImmutableSkinningMatrixPalette
{
    SkinningMatrixPalette mPalette;

public:
    ImmutableSkinningMatrixPalette(SkinningMatrixPalette palette)
        : mPalette(std::move(palette))
    { }

    const SkinningMatrixPalette& GetPalette() const
    {
        return mPalette;
    }
};

static_assert(sizeof(SkeletonJointPose) == 12 * sizeof(float),
              "JointPose should be aligned to vec4 size");

void GlobalPosesToSkinningMatrices(
        const SkeletonJoint* joints,
        const mat4* NG_RESTRICT globalPoses,
        std::size_t numJoints,
        mat4* NG_RESTRICT skinningMatrices);

} // end namespace ng

#endif // NG_SKELETALMODEL_HPP
