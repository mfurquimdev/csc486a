#ifndef NG_MATERIAL_HPP
#define NG_MATERIAL_HPP

#include "ng/engine/rendering/sampler.hpp"

#include "ng/engine/math/linearalgebra.hpp"

#include <memory>

namespace ng
{

class ITexture;

enum class MaterialType
{
    Null,
    Colored,
    NormalColored,
    VertexColored,
    Textured,
    Wireframe
};

class Material
{
public:
    MaterialType Type;

    vec3 Tint;

    std::shared_ptr<ITexture> Texture0;
    Sampler Sampler0;

    Material()
        : Type(MaterialType::Null)
    { }

    Material(MaterialType type)
        : Type(type)
    { }
};

} // end namespace ng

#endif // NG_MATERIAL_HPP
