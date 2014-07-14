#ifndef NG_MATERIAL_HPP
#define NG_MATERIAL_HPP

#include "ng/engine/math/linearalgebra.hpp"

namespace ng
{

enum class MaterialType
{
    Null,
    Colored,
    NormalColored,
    Wireframe
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

struct WireframeMaterial
{
    vec3 Tint;
};

class Material
{
public:
    MaterialType Type = MaterialType::Null;

    union {
        NullMaterial Null;
        ColoredMaterial Colored;
        NormalColoredMaterial NormalColored;
        WireframeMaterial Wireframe;
    };

    Material()
        : Null()
    { }
};

} // end namespace ng

#endif // NG_MATERIAL_HPP
