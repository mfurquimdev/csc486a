#ifndef NG_TEXTURE_HPP
#define NG_TEXTURE_HPP

#include "ng/engine/rendering/textureformat.hpp"

#include <cstddef>

namespace ng
{

class ITexture
{
public:
    virtual ~ITexture() = default;

    virtual TextureFormat GetTextureFormat() const = 0;

    virtual std::size_t WriteTextureData(void* buffer) const = 0;
};

} // end namespace ng

#endif // NG_TEXTURE_HPP
