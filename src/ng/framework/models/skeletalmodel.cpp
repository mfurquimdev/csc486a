#include "ng/framework/models/skeletalmodel.hpp"

#include "ng/framework/models/md5model.hpp"

#include "ng/engine/util/debug.hpp"

namespace ng
{

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
        int frameIndex)
{
    SkeletonLocalPose localPose;

    if (skeleton.Joints.size() != anim.Joints.size())
    {
        throw std::logic_error(
                    "incompatible number of joints "
                    "between skeleton and animation.");
    }

    if (frameIndex || frameIndex >= (int) anim.Frames.size())
    {
        throw std::logic_error("frame out of bounds");
    }

    const MD5Frame& frame = anim.Frames[frameIndex];

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

        SkeletonJointPose localPoseJoint;
        localPoseJoint.Translation = basePoseJoint.Position;

        Quaternionf quat;
        quat.Components.x = basePoseJoint.Orientation.x;
        quat.Components.y = basePoseJoint.Orientation.y;
        quat.Components.z = basePoseJoint.Orientation.z;

        // apply the transformation induced by the current frame
        unsigned int flags = animationJoint.Flags;

        int frameDataOffset = animationJoint.StartIndex;

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

        DebugPrintf("quat xyz: {%f,%f,%f}\n",
                    quat.Components[0], quat.Components[1], quat.Components[2]);

        float t = 1.0f - dot(vec3(quat.Components),
                             vec3(quat.Components));
        if (t < 0.0f)
        {
            quat.Components.w = 0.0f;
        }
        else
        {
            quat.Components.w = std::sqrt(t);
        }

        localPoseJoint.Rotation = quat;

        localPose.JointPoses.push_back(localPoseJoint);
    }

    return std::move(localPose);
}

SkeletonGlobalPose SkeletonGlobalPose::FromLocalPose(
        const Skeleton& skeleton,
        const SkeletonLocalPose& localPose)
{
    SkeletonGlobalPose globalPose;

    if (skeleton.Joints.size() != localPose.JointPoses.size())
    {
        throw std::logic_error(
                    "mismatch between number of joints in skeleton "
                    "and the number of joints in the local pose");
    }

    // note: we can assume that the parent's global transform
    //       has already been computed, since joints always have their parent
    //       before themselves.
    for (std::size_t j = 0; j < skeleton.Joints.size(); j++)
    {
        const SkeletonJoint& joint = skeleton.Joints[j];
        const SkeletonJointPose& localJoint = localPose.JointPoses[j];

        SkeletonJointPose globalJoint;

        if (joint.ParentIndex == SkeletonJoint::RootJointIndex)
        {
            globalJoint = localJoint;
        }
        else
        {
            const SkeletonJointPose& parentJoint =
                    globalPose.GlobalPoses[joint.ParentIndex];

            globalJoint.Scale = parentJoint.Scale * localJoint.Scale;

            globalJoint.Rotation = normalize(
                        parentJoint.Rotation * localJoint.Rotation);

            globalJoint.Translation = vec3(rotate(
                        parentJoint.Rotation,
                        parentJoint.Scale * localJoint.Translation).Components)
                    + parentJoint.Translation;
        }

        DebugPrintf("localJoint.Rotation = \n"
                    "{%f,%f,%f,%f}\n",
                    localJoint.Rotation.Components[0],
                    localJoint.Rotation.Components[1],
                    localJoint.Rotation.Components[2],
                    localJoint.Rotation.Components[3]);

        DebugPrintf("localJoint.Scale = \n"
                    "{%f,%f,%f}\n",
                    localJoint.Scale[0],
                    localJoint.Scale[1],
                    localJoint.Scale[2]);

        DebugPrintf("localJoint.Translation = \n"
                    "{%f,%f,%f}\n",
                    localJoint.Translation[0],
                    localJoint.Translation[1],
                    localJoint.Translation[2]);

        DebugPrintf("globalJoint.Rotation = \n"
                    "{%f,%f,%f,%f}\n",
                    globalJoint.Rotation.Components[0],
                    globalJoint.Rotation.Components[1],
                    globalJoint.Rotation.Components[2],
                    globalJoint.Rotation.Components[3]);

        DebugPrintf("globalJoint.Scale = \n"
                    "{%f,%f,%f}\n",
                    globalJoint.Scale[0],
                    globalJoint.Scale[1],
                    globalJoint.Scale[2]);

        DebugPrintf("globalJoint.Translation = \n"
                    "{%f,%f,%f}\n",
                    globalJoint.Translation[0],
                    globalJoint.Translation[1],
                    globalJoint.Translation[2]);

        globalPose.GlobalPoses.push_back(globalJoint);
    }

    return std::move(globalPose);
}

SkinningMatrixPalette SkinningMatrixPalette::FromGlobalPose(
        const Skeleton& skeleton,
        const SkeletonGlobalPose& globalPose)
{
    SkinningMatrixPalette palette;

    if (skeleton.Joints.size() != globalPose.GlobalPoses.size())
    {
        throw std::logic_error(
                    "Mismatch between number of skeleton joints "
                    "and number of poses in the global pose");
    }

    for (std::size_t j = 0; j < skeleton.Joints.size(); j++)
    {
        const SkeletonJointPose& globalJoint = globalPose.GlobalPoses[j];

        mat3 R(globalJoint.Rotation);
        mat3 S(scale3x3(globalJoint.Scale));
        mat4 P(R * S);
        P[3][0] = globalJoint.Translation[0];
        P[3][1] = globalJoint.Translation[1];
        P[3][2] = globalJoint.Translation[2];

        palette.SkinningMatrices.push_back(
                    P * skeleton.Joints[j].InverseBindPose);
    }

    return std::move(palette);
}

} // end namespace ng
