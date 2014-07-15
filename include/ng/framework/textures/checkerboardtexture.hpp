#ifndef NG_CHECKERBOARDTEXTURE_HPP
#define NG_CHECKERBOARDTEXTURE_HPP

#include "ng/engine/rendering/texture.hpp"

#include "ng/engine/math/linearalgebra.hpp"

namespace ng
{

class CheckerboardTexture : public ITexture
{
    const int mHorizontalTiles;
    const int mVerticalTiles;
    const int mTileSizeInPixels;
    const vec4 mColor1;
    const vec4 mColor2;

public:
    CheckerboardTexture(
        int horizontalTiles,
        int verticalTiles,
        int tileSizeInPixels,
        vec4 color1,
        vec4 color2);

    TextureFormat GetTextureFormat() const override;

    std::size_t WriteTextureData(void* buffer) const override;
};

} // end namespace ng

#endif // NG_CHECKERBOARDTEXTURE_HPP
