#ifndef NG_OBJMODEL_HPP
#define NG_OBJMODEL_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <vector>
#include <string>

namespace ng
{

class ObjShape
{
public:
    std::string Name;

    std::vector<vec3> Positions;
    std::vector<vec2> Texcoords;
    std::vector<vec3> Normals;

    std::vector<int> Indices;
    bool HasPositionIndices = false;
    bool HasTexcoordIndices = false;
    bool HasNormalIndices = false;
    int VerticesPerFace = 0;

    // convenience functions
    int GetIndicesPerVertex() const
    {
        return int(HasPositionIndices)
             + int(HasTexcoordIndices)
             + int(HasNormalIndices);
    }
};


} // end namespace ng

#endif // NG_OBJMODEL_HPP
