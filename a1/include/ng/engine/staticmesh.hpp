#ifndef NG_STATICMESH_HPP
#define NG_STATICMESH_HPP

#include <memory>

namespace ng
{

struct VertexFormat;

class IStaticMesh
{
public:
    virtual ~IStaticMesh() = default;

    virtual void Init(const VertexFormat& format,
                      std::shared_ptr<const void> vertexData,
                      std::ptrdiff_t vertexDataSize,
                      std::shared_ptr<const void> indexData,
                      std::ptrdiff_t indexDataSize) = 0;
};

} // end namespace ng

#endif // NG_STATICMESH_HPP
