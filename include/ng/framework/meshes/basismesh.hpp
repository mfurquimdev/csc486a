#ifndef NG_BASISMESH_HPP
#define NG_BASISMESH_HPP

#include "ng/engine/rendering/mesh.hpp"

namespace ng
{

class BasisMesh : public IMesh
{
    class Vertex;

public:
    VertexFormat GetVertexFormat() const override;

    std::size_t GetMaxVertexBufferSize() const override;
    std::size_t GetMaxIndexBufferSize() const override;

    std::size_t WriteVertices(void* buffer) const override;
    std::size_t WriteIndices(void* buffer) const override;
};

} // end namespace ng

#endif // NG_BASISMESH_HPP
