#ifndef NG_STATICMESH_HPP
#define NG_STATICMESH_HPP

#include "ng/engine/rendering/renderstate.hpp"
#include "ng/engine/rendering/uniform.hpp"
#include "ng/engine/rendering/vertexformat.hpp"
#include "ng/engine/rendering/primitivetype.hpp"

#include <memory>
#include <map>

namespace ng
{

class IShaderProgram;

class IStaticMesh
{
public:
    virtual ~IStaticMesh() = default;

    // discard all state
    virtual void Reset() = 0;

    virtual void Init(VertexFormat format,
                      std::map<VertexAttributeName, std::pair<std::shared_ptr<const void>,std::ptrdiff_t>> attributeDataAndSize,
                      std::shared_ptr<const void> indexData,
                      std::ptrdiff_t indexDataSize,
                      std::size_t vertexCount) = 0;

    virtual void Draw(std::shared_ptr<IShaderProgram> program,
                      std::map<std::string, UniformValue> uniforms,
                      RenderState renderState,
                      PrimitiveType primitiveType, std::size_t firstVertexIndex, std::size_t vertexCount) = 0;

    virtual std::size_t GetVertexCount() const = 0;
};

} // end namespace ng

#endif // NG_STATICMESH_HPP
