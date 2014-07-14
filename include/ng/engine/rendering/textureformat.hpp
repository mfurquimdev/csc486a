#ifndef NG_TEXTUREFORMAT_HPP
#define NG_TEXTUREFORMAT_HPP

namespace ng
{

enum class TextureType
{
    Invalid,
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture3D,
    TextureCubeMap,
    TextureCubeMapArray
};

enum class ImageFormat
{
    Invalid,
    RGBA8
};

class TextureFormat
{
public:
    ImageFormat Format = ImageFormat::Invalid;

    TextureType Type = TextureType::Invalid;

    int Width;
    int Height;
    int Depth;

    bool EnableMipMapping = false;
};

} // end namespace ng

#endif // NG_TEXTUREFORMAT_HPP
