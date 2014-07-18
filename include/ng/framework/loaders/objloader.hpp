#ifndef NG_OBJLOADER_HPP
#define NG_OBJLOADER_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <vector>
#include <string>

namespace ng
{

class IReadFile;

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

bool TryLoadObj(
        ObjShape& shape,
        IReadFile& objFile,
        std::string& error);

void LoadObj(
        ObjShape& shape,
        IReadFile& objFile);

} // end namespace ng

#endif // NG_OBJLOADER_HPP
