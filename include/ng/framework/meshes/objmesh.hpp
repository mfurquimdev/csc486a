#ifndef NG_OBJMESH_HPP
#define NG_OBJMESH_HPP

#include "ng/engine/rendering/mesh.hpp"
#include "ng/framework/loaders/objloader.hpp"

namespace ng
{

class ObjMesh : public IMesh
{
    const ObjShape mShape;

public:
    ObjMesh(ObjShape shape);

    VertexFormat GetVertexFormat() const override;

    std::size_t GetMaxVertexBufferSize() const override;
    std::size_t GetMaxIndexBufferSize() const override;

    std::size_t WriteVertices(void* buffer) const override;
    std::size_t WriteIndices(void* buffer) const override;
};

} // end namespace ng

#endif // NG_OBJMESH_HPP
