#ifndef NG_OBJMODEL_HPP
#define NG_OBJMODEL_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <vector>
#include <string>

namespace ng
{

class ObjModel
{
public:
    std::string Name;

    std::vector<vec3> Positions;
    std::vector<vec2> Texcoords;
    std::vector<vec3> Normals;

    std::vector<vec<std::uint8_t,4>> JointIndices;
    std::vector<vec3> JointWeights;

    std::vector<int> Indices;

    bool HasPositionIndices = false;
    bool HasTexcoordIndices = false;
    bool HasNormalIndices = false;

    bool HasJointIndexIndices = false;
    bool HasJointWeightIndices = false;

    int VerticesPerFace = 0;

    // convenience functions
    int GetIndicesPerVertex() const
    {
        return int(HasPositionIndices)
             + int(HasTexcoordIndices)
             + int(HasNormalIndices)
             + int(HasJointIndexIndices)
             + int(HasJointWeightIndices);
    }
};

} // end namespace ng

#endif // NG_OBJMODEL_HPP
