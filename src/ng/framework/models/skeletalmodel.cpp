#include "ng/framework/models/skeletalmodel.hpp"

namespace ng
{

mat4 PoseToMat4(const SkeletonJointPose& pose)
{
    mat3 R(pose.Rotation);
    mat3 S(scale3x3(pose.Scale));
    mat4 P(R * S);
    P[3][0] = pose.Translation[0];
    P[3][1] = pose.Translation[1];
    P[3][2] = pose.Translation[2];
    return P;
}

void LocalPosesToGlobalPoses(
        const SkeletonJoint* joints,
        const SkeletonJointPose* localPoses,
        std::size_t numJoints,
        mat4* globalPoses)
{
    for (std::size_t j = 0; j < numJoints; j++)
    {
        const SkeletonJoint* joint = &joints[j];
        const SkeletonJointPose* pose = &localPoses[j];

        mat4 Pj_M = PoseToMat4(*pose);
        while (joint->Parent != -1)
        {
            int parent = joint->Parent;
            joint = &joints[parent];
            pose = &localPoses[parent];

            Pj_M = PoseToMat4(*pose) * Pj_M;
        }

        globalPoses[j] = Pj_M;
    }
}

void GlobalPosesToSkinningMatrices(
        const SkeletonJoint* joints,
        const mat4* NG_RESTRICT globalPoses,
        std::size_t numJoints,
        mat4* NG_RESTRICT skinningMatrices)
{
    for (std::size_t j = 0; j < numJoints; j++)
    {
        skinningMatrices[j] = globalPoses[j]
                            * joints[j].InverseBindPose;
    }
}

vec3 BindPoseToCurrentPose(
        const mat4* skinningMatrices,
        const float* jointWeights,
        const std::size_t* skinningIndices,
        std::size_t numJoints,
        vec3 bindPoseVertex)
{
    vec4 bindPose4(bindPoseVertex, 1.0f);
    vec4 result;

    for (std::size_t j = 0; j < numJoints; j++)
    {
        result += jointWeights[j]
                * (skinningMatrices[skinningIndices[j]] * bindPose4);
    }

    return vec3(result);
}

} // end namespace ng
