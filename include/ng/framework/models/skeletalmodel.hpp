#ifndef NG_SKELETALMODEL_HPP
#define NG_SKELETALMODEL_HPP

#include "ng/engine/math/linearalgebra.hpp"
#include "ng/engine/math/quaternion.hpp"
#include "ng/engine/util/compilertraits.hpp"

#include <vector>
#include <string>

namespace ng
{

class MD5Model;
class MD5Anim;

class Skeleton
{
public:
    class Joint
    {
    public:
        mat4 InverseBindPose;
        std::string JointName;

        static constexpr int RootJointIndex = -1;
        int ParentIndex;
    };

    static Skeleton FromMD5Model(const MD5Model& model);

    std::vector<Joint> Joints;
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

class SkeletonLocalPose
{
public:
    static SkeletonLocalPose FromMD5AnimFrame(
            const Skeleton& skeleton,
            const MD5Anim& anim,
            int frame);

    std::vector<SkeletonJointPose> JointPoses;
};

class SkeletonGlobalPose
{
public:
    static SkeletonGlobalPose FromLocalPose(
            const Skeleton& skeleton,
            const SkeletonLocalPose& localPose);

    std::vector<SkeletonJointPose> GlobalPoses;
};

class SkinningMatrixPalette
{
public:
    static SkinningMatrixPalette FromGlobalPose(
            const Skeleton& skeleton,
            const SkeletonGlobalPose& globalPose);

    std::vector<mat4> SkinningMatrices;
};

} // end namespace ng

#endif // NG_SKELETALMODEL_HPP
