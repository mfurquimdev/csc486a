#ifndef NG_MATERIAL_HPP
#define NG_MATERIAL_HPP

#include "ng/engine/math/linearalgebra.hpp"

namespace ng
{

enum class MaterialType
{
    Null,
    Colored,
    NormalColored
};

struct NullMaterial
{
};

struct ColoredMaterial
{
    vec3 Tint;
};

struct NormalColoredMaterial
{
};

class Material
{
public:
    MaterialType Type = MaterialType::Null;

    union {
        NullMaterial Null;
        ColoredMaterial Colored;
        NormalColoredMaterial NormalColored;
    };

    Material()
        : Null()
    { }
};

} // end namespace ng

#endif // NG_MATERIAL_HPP
