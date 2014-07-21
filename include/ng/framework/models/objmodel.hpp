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

    std::vector<vec4> Positions;
    std::vector<vec3> Texcoords;
    std::vector<vec3> Normals;

    std::vector<int> Indices;
    bool HasPositionIndices = false;
    bool HasTexcoordIndices = false;
    bool HasNormalIndices = false;

    int VerticesPerFace = 0;
};

} // end namespace ng

#endif // NG_OBJMODEL_HPP
