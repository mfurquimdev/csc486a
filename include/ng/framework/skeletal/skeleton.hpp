#ifndef NG_SKELETON_HPP
#define NG_SKELETON_HPP

#include "ng/engine/math/linearalgebra.hpp"
#include "ng/engine/math/quaternion.hpp"
#include "ng/engine/util/compilertraits.hpp"

#include <vector>

namespace ng
{

class Joint
{
public:
    mat4 InverseBindPose;
    std::string JointName;

    // -1 if this is the root.
    int Parent;
};

class JointPose
{
public:
    Quaternionf Rotation;
    vec3 Translation;
    vec3 Scale{1};
    std::uint8_t Padding[8];
};

static_assert(sizeof(JointPose) == 12 * sizeof(float),
              "JointPose should be aligned to vec4 size");

mat4 PoseToMat4(const JointPose& pose);

void CalculateGlobalPoses(
        const Joint* joints,
        const JointPose* localPoses,
        std::size_t numJoints,
        mat4* globalPoses);

void CalculateSkinningMatrixPalette(
        const Joint* joints,
        const mat4* NG_RESTRICT globalPoses,
        std::size_t numJoints,
        mat4* NG_RESTRICT skinningMatrices);

vec3 CalculateCurrentPoseVertex(
        const mat4* skinningMatrices,
        const float* jointWeights,
        std::size_t numJoints,
        vec4 bindPoseVertex);

} // end namespace ng

#endif // NG_SKELETON_HPP
