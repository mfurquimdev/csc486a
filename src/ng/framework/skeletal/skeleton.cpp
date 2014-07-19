#include "ng/framework/skeletal/skeleton.hpp"

namespace ng
{

mat4 PoseToMat4(const JointPose& pose)
{
    mat3 R(pose.Rotation);
    mat3 S(scale3x3(pose.Scale));
    mat4 P(R * S);
    P[3][0] = pose.Translation[0];
    P[3][1] = pose.Translation[1];
    P[3][2] = pose.Translation[2];
    return P;
}

void CalculateGlobalPoses(
        const Joint* joints,
        const JointPose* localPoses,
        std::size_t numJoints,
        mat4* globalPoses)
{
    for (std::size_t j = 0; j < numJoints; j++)
    {
        const Joint* joint = &joints[j];
        const JointPose* pose = &localPoses[j];

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

void CalculateSkinningMatrixPalette(
        const Joint* joints,
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

} // end namespace ng
