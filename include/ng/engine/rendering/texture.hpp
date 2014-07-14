#ifndef NG_TEXTURE_HPP
#define NG_TEXTURE_HPP

#include "ng/engine/rendering/textureformat.hpp"

namespace ng
{

class ITexture
{
public:
    virtual TextureFormat GetTextureFormat() const = 0;

    virtual void WriteTextureData(void* buffer) const = 0;
};

} // end namespace ng

#endif // NG_TEXTURE_HPP
