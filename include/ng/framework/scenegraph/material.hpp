#ifndef NG_MATERIAL_HPP
#define NG_MATERIAL_HPP

#include <functional>

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
    Material() = default;

    Material(MaterialStyle style,
             MaterialQuality quality)
        : Style(style)
        , Quality(quality)
    { }

    MaterialStyle Style = MaterialStyle::Phong;
    MaterialQuality Quality = MaterialQuality::Medium;
};

struct MaterialLess
{
    constexpr bool operator()(const ng::Material& a, const ng::Material& b) const
    {
        return (int(a.Style) < int(b.Style)) ||
               (int(a.Style) == int(b.Style) && int(a.Quality) < int(b.Quality));
    }
};

} // end namespace ng

#endif // NG_MATERIAL_HPP
