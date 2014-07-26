#ifndef NG_MD5MODEL_HPP
#define NG_MD5MODEL_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <string>
#include <vector>

namespace ng
{

class MD5Joint
{
public:
    std::string Name;
    int ParentIndex;
    vec3 Position;
    vec3 Orientation;
};

class MD5Vertex
{
public:
    vec2 Texcoords;
    int StartWeight;
    int WeightCount;
};

class MD5Triangle
{
public:
    ivec3 VertexIndices;
};

class MD5Weight
{
public:
    int JointIndex;
    float WeightBias;
    vec3 WeightPosition;
};

class MD5MeshData
{
public:
    std::string Shader;
    std::vector<MD5Vertex> Vertices;
    std::vector<MD5Triangle> Triangles;
    std::vector<MD5Weight> Weights;
};

class MD5Model
{
public:
    int MD5Version;
    std::string CommandLine;

    std::vector<MD5Joint> BindPoseJoints;
    std::vector<MD5MeshData> Meshes;
};

class MD5AnimationJoint
{
public:
    static constexpr unsigned int PositionXFlag = 1;
    static constexpr unsigned int PositionYFlag = 2;
    static constexpr unsigned int PositionZFlag = 4;
    static constexpr unsigned int QuaternionXFlag = 8;
    static constexpr unsigned int QuaternionYFlag = 16;
    static constexpr unsigned int QuaternionZFlag = 32;

    std::string Name;
    int ParentIndex;
    unsigned int Flags;
    int StartIndex;
};

class MD5FrameBounds
{
public:
    vec3 MinimumExtent;
    vec3 MaximumExtent;
};

class MD5JointPose
{
public:
    vec3 Position;
    vec3 Orientation;
};

class MD5Frame
{
public:
    std::vector<float> AnimationComponents;
};

class MD5Anim
{
public:
    int MD5Version;
    std::string CommandLine;
    int FrameRate;
    std::vector<MD5AnimationJoint> Joints;
    std::vector<MD5FrameBounds> FrameBounds;
    std::vector<MD5JointPose> BaseFrame;
    std::vector<MD5Frame> Frames;
};



} // end namespace ng

#endif // NG_MD5MODEL_HPP
