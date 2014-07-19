#ifndef NG_JOINTS_HPP
#define NG_JOINTS_HPP

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

    // -1 if this is the root.
    int Parent;
};

class SkeletonJointPose
{
public:
    Quaternionf Rotation;
    vec3 Translation;
    vec3 Scale{1};

private:
    std::uint8_t Padding[8];
};

static_assert(sizeof(SkeletonJointPose) == 12 * sizeof(float),
              "JointPose should be aligned to vec4 size");

mat4 PoseToMat4(const SkeletonJointPose& pose);

void LocalPosesToGlobalPoses(
        const SkeletonJoint* joints,
        const SkeletonJointPose* localPoses,
        std::size_t numJoints,
        mat4* globalPoses);

void GlobalPosesToSkinningMatrices(
        const SkeletonJoint* joints,
        const mat4* NG_RESTRICT globalPoses,
        std::size_t numJoints,
        mat4* NG_RESTRICT skinningMatrices);

vec3 BindPoseToCurrentPose(
        const mat4* skinningMatrices,
        const float* jointWeights,
        const std::size_t* skinningIndices,
        std::size_t numJoints,
        vec3 bindPoseVertex);

} // end namespace ng

#endif // NG_JOINTS_HPP
