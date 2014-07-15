#ifndef NG_SAMPLER_HPP
#define NG_SAMPLER_HPP

namespace ng
{

enum class TextureFilter
{
    Invalid,
    Linear,
    Nearest
};

enum class TextureWrap
{
    Invalid,
    Repeat,
    ClampToEdge
};

class Sampler
{
public:
    TextureFilter MinFilter = TextureFilter::Invalid;
    TextureFilter MagFilter = TextureFilter::Invalid;

    TextureWrap WrapX = TextureWrap::Invalid;
    TextureWrap WrapY = TextureWrap::Invalid;
    TextureWrap WrapZ = TextureWrap::Invalid;
};

} // end namespace ng

#endif // NG_SAMPLER_HPP
