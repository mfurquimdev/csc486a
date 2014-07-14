#ifndef NG_LOOPSUBDIVISIONMESH_HPP
#define NG_LOOPSUBDIVISIONMESH_HPP

#include "ng/engine/rendering/mesh.hpp"

#include <memory>

namespace ng
{

class LoopSubdivisionMesh : public IMesh
{
    const std::shared_ptr<const IMesh> mMeshToSubdivide;
    int mNumSubdivisons;

public:
    LoopSubdivisionMesh(
            std::shared_ptr<IMesh> meshToSubdivide,
            int numSubdivisions);

    VertexFormat GetVertexFormat() const override;

    std::size_t GetMaxVertexBufferSize() const override;
    std::size_t GetMaxIndexBufferSize() const override;

    std::size_t WriteVertices(void* buffer) const override;
    std::size_t WriteIndices(void* buffer) const override;
};

} // end namespace ng

#endif // NG_LOOPSUBDIVISIONMESH_HPP
