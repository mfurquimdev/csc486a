#ifndef NG_STATICMESH_HPP
#define NG_STATICMESH_HPP

#include <memory>
#include <vector>

namespace ng
{

struct VertexFormat;

class IStaticMesh
{
public:
    virtual ~IStaticMesh() = default;

    virtual void Init(const VertexFormat& format,
                      const std::vector<std::pair<std::shared_ptr<const void>,std::ptrdiff_t>>& vertexDataAndSize,
                      std::shared_ptr<const void> indexData,
                      std::ptrdiff_t indexDataSize) = 0;
};

} // end namespace ng

#endif // NG_STATICMESH_HPP
