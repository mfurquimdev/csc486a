#ifndef NG_STATICMESH_HPP
#define NG_STATICMESH_HPP

#include "ng/engine/primitivetype.hpp"

#include <memory>
#include <vector>

namespace ng
{

struct VertexFormat;
class IShaderProgram;

class IStaticMesh
{
public:
    virtual ~IStaticMesh() = default;

    virtual void Init(const VertexFormat& format,
                      const std::vector<std::pair<std::shared_ptr<const void>,std::ptrdiff_t>>& vertexDataAndSize,
                      std::shared_ptr<const void> indexData,
                      std::ptrdiff_t indexDataSize,
                      std::size_t vertexCount) = 0;

    // TODO: Pass uniforms somehow
    virtual void Draw(const std::shared_ptr<IShaderProgram>& program,
                      PrimitiveType primitiveType, std::size_t firstVertexIndex, std::size_t vertexCount) = 0;

    virtual std::size_t GetVertexCount() const = 0;
};

} // end namespace ng

#endif // NG_STATICMESH_HPP
