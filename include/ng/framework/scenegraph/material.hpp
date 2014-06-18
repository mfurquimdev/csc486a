#ifndef NG_MATERIAL_HPP
#define NG_MATERIAL_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <functional>
#include <algorithm>

namespace ng
{

enum class MaterialStyle : int
{
    Phong,
    Debug
};

enum class MaterialQuality : int
{
    Medium
};

class Material
{
public:
    MaterialStyle Style = MaterialStyle::Debug;

    MaterialQuality Quality = MaterialQuality::Medium;

    vec3 Tint = vec3(1,1,1);
};

} // end namespace ng

#endif // NG_MATERIAL_HPP
