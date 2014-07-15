#ifndef NG_TEXTUREFORMAT_HPP
#define NG_TEXTUREFORMAT_HPP

namespace ng
{

enum class TextureType
{
    Invalid,
    Texture2D,
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

    int Width = 1;
    int Height = 1;
    int Depth = 1;

    bool EnableMipMapping = false;
};

} // end namespace ng

#endif // NG_TEXTUREFORMAT_HPP
