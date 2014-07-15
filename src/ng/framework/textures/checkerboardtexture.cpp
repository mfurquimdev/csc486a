#include "ng/framework/textures/checkerboardtexture.hpp"

#include <stdexcept>

namespace ng
{

CheckerboardTexture::CheckerboardTexture(
        int horizontalTiles,
        int verticalTiles,
        int tileSizeInPixels,
        vec4 color1,
        vec4 color2)

    : mHorizontalTiles(horizontalTiles)
    , mVerticalTiles(verticalTiles)
    , mTileSizeInPixels(tileSizeInPixels)
    , mColor1(color1)
    , mColor2(color2)
{
    if (mHorizontalTiles <= 0)
    {
        throw std::logic_error("horizontalTiles must be > 0");
    }

    if (mVerticalTiles <= 0)
    {
        throw std::logic_error("verticalTiles must be > 0");
    }

    if (mTileSizeInPixels <= 0)
    {
        throw std::logic_error("tileSizeInPixels must be > 0");
    }

    if (std::any_of(&mColor1[0], &mColor1[0] + 4,
    [](float x) { return x < 0.0f || x > 1.0f; }))
    {
        throw std::logic_error("color1 is outside the 0-1 range");
    }

    if (std::any_of(&mColor2[0], &mColor2[0] + 4,
    [](float x) { return x < 0.0f || x > 1.0f; }))
    {
        throw std::logic_error("color2 is outside the 0-1 range");
    }
}

TextureFormat CheckerboardTexture::GetTextureFormat() const
{
    TextureFormat fmt;
    fmt.Format = ImageFormat::RGBA8;
    fmt.Type = TextureType::Texture2D;
    fmt.Width = mHorizontalTiles * mTileSizeInPixels;
    fmt.Height = mVerticalTiles * mTileSizeInPixels;
    fmt.EnableMipMapping = false;

    return fmt;
}

std::size_t CheckerboardTexture::WriteTextureData(void* buffer) const
{
    using pixel = vec<std::uint8_t,4>;

    pixel* pixels = static_cast<pixel*>(buffer);

    const pixel colors[2] = {
        pixel(mColor1 * 255.0f),
        pixel(mColor2 * 255.0f)
    };

    // note: GL starts from the bottom!
    for (int vTile = 0; vTile < mVerticalTiles; vTile++)
    {
        int rowFirstColor = vTile % 2;

        for (int vTileOffset = 0; vTileOffset < mTileSizeInPixels; vTileOffset++)
        {
            for (int hTile = 0; hTile < mHorizontalTiles; hTile++)
            {
                int color = (rowFirstColor + hTile) % 2;

                for (int hTileOffset = 0; hTileOffset < mTileSizeInPixels; hTileOffset++)
                {
                    std::size_t idx =
                        (vTile * mTileSizeInPixels + vTileOffset)
                            * mHorizontalTiles * mTileSizeInPixels
                        + hTile * mTileSizeInPixels + hTileOffset;

                    pixels[idx] = colors[color];
                }
            }
        }
    }

    return mVerticalTiles * mHorizontalTiles * mTileSizeInPixels;
}

} // end namespace ng
