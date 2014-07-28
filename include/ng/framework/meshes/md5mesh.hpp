#ifndef NG_MD5MESH_HPP
#define NG_MD5MESH_HPP

#include "ng/engine/rendering/mesh.hpp"
#include "ng/framework/models/md5model.hpp"

namespace ng
{

class MD5Mesh : public IMesh
{
    class Vertex;

    const MD5Model mModel;

public:
    MD5Mesh(MD5Model model);

    VertexFormat GetVertexFormat() const override;

    std::size_t GetMaxVertexBufferSize() const override;
    std::size_t GetMaxIndexBufferSize() const override;

    std::size_t WriteVertices(void* buffer) const override;
    std::size_t WriteIndices(void* buffer) const override;
};

} // end namespace ng

#endif // NG_MD5MESH_HPP
