#include "ng/framework/models/skeletalmodel.hpp"

#include "ng/framework/models/md5model.hpp"

namespace ng
{

namespace
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

void CalculateInverseBindPose(
        const SkeletonJointPose* bindPoseJointPoses,
        SkeletonJoint* joints,
        std::size_t numJoints)
{
    for (std::size_t j = 0; j < numJoints; j++)
    {
        const SkeletonJoint* joint = &joints[j];
        const SkeletonJointPose* pose = &bindPoseJointPoses[j];

        mat4 Pj_M = PoseToMat4(*pose);
        while (joint->ParentIndex != SkeletonJoint::RootJointIndex)
        {
            int parent = joint->ParentIndex;
            joint = &joints[parent];
            pose = &bindPoseJointPoses[parent];

            Pj_M = PoseToMat4(*pose) * Pj_M;
        }

        joints[j].InverseBindPose = inverse(Pj_M);
    }
}

} // end anonymous namespace

Skeleton Skeleton::FromMD5Model(const MD5Model& model)
{
    Skeleton skeleton;

    // use the model's bind pose
    // to create the inverse bind pose matrices.

    for (const MD5Joint& md5Joint : model.BindPoseJoints)
    {
        SkeletonJoint joint;
        joint.JointName = md5Joint.Name;
        joint.ParentIndex = md5Joint.ParentIndex;

        Quaternionf quat;
        quat.Components.x = md5Joint.Orientation.x;
        quat.Components.y = md5Joint.Orientation.y;
        quat.Components.z = md5Joint.Orientation.z;

        float t = 1.0f - dot(md5Joint.Orientation, md5Joint.Orientation);
        if (t < 0.0f)
        {
            quat.Components.w = 0.0f;
        }
        else
        {
            quat.Components.w = -std::sqrt(t);
        }

        mat4 invBindPose = mat4(transpose(mat3(quat)));
        invBindPose[3][0] = -md5Joint.Position.x;
        invBindPose[3][1] = -md5Joint.Position.y;
        invBindPose[3][2] = -md5Joint.Position.z;

        joint.InverseBindPose = invBindPose;

        skeleton.Joints.push_back(std::move(joint));
    }

    return std::move(skeleton);
}

SkeletonLocalPose SkeletonLocalPose::FromMD5AnimFrame(
        const Skeleton& skeleton,
        const MD5Anim& anim,
        int frame)
{
    SkeletonLocalPose localPose;

    if (skeleton.Joints.size() != anim.Joints.size())
    {
        throw std::logic_error(
                    "incompatible number of joints "
                    "between skeleton and animation.");
    }

    if (frame < 0 || frame >= anim.Frames.size())
    {
        throw std::logic_error("frame out of bounds");
    }

    for (std::size_t j = 0; j < skeleton.Joints.size(); j++)
    {
        if (skeleton.Joints[j].ParentIndex != anim.Joints[j].ParentIndex)
        {
            throw std::logic_error(
                        "incompatible joint hierarchy "
                        "between skeleton and animation");
        }

        const MD5AnimationJoint& animationJoint = anim.Joints[j];
        const MD5JointPose& basePoseJoint = anim.BaseFrame[j];
        const MD5Frame& frame = anim.Frames[j];

        SkeletonJointPose localPoseJoint;
        localPoseJoint.Translation = basePoseJoint.Position;

        Quaternion quat;
        quat.Components.x = basePoseJoint.Orientation.x;
        quat.Components.y = basePoseJoint.Orientation.y;
        quat.Components.z = basePoseJoint.Orientation.z;

        // apply the transformation induced by the current frame
        unsigned int flags = animationJoint.Flags;

        int frameDataOffset = 0;

        if (flags & MD5AnimationJoint::PositionXFlag)
        {
            localPoseJoint.Translation[0] =
                    frame.AnimationComponents[frameDataOffset];
            frameDataOffset++;
        }

        if (flags & MD5AnimationJoint::PositionYFlag)
        {
            localPoseJoint.Translation[1] =
                    frame.AnimationComponents[frameDataOffset];
            frameDataOffset++;
        }

        if (flags & MD5AnimationJoint::PositionZFlag)
        {
            localPoseJoint.Translation[2] =
                    frame.AnimationComponents[frameDataOffset];
            frameDataOffset++;
        }

        if (flags & MD5AnimationJoint::QuaternionXFlag)
        {
            quat.Components[0] =
                    frame.AnimationComponents[frameDataOffset];
            frameDataOffset++;
        }

        if (flags & MD5AnimationJoint::QuaternionYFlag)
        {
            quat.Components[1] =
                    frame.AnimationComponents[frameDataOffset];
            frameDataOffset++;
        }

        if (flags & MD5AnimationJoint::QuaternionZFlag)
        {
            quat.Components[2] =
                    frame.AnimationComponents[frameDataOffset];
            frameDataOffset++;
        }

        float t = 1.0f - dot(vec3(quat.Components),
                             vec3(quat.Components));
        if (t < 0.0f)
        {
            quat.Components.w = 0.0f;
        }
        else
        {
            quat.Components.w = -std::sqrt(t);
        }

        localPoseJoint.Rotation = quat;

        localPose.JointPoses.push_back(localPoseJoint);
    }

    return std::move(localPose);
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
        while (joint->ParentIndex != SkeletonJoint::RootJointIndex)
        {
            int parent = joint->ParentIndex;
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

} // end namespace ng
