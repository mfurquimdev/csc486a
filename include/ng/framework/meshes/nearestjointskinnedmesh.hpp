#ifndef NG_NEARESTJOINTSKINNEDMESH_HPP
#define NG_NEARESTJOINTSKINNEDMESH_HPP

#include "ng/engine/rendering/mesh.hpp"

#include <memory>

namespace ng
{

class ImmutableSkeleton;

class NearestJointSkinnedMesh : public IMesh
{
    std::shared_ptr<IMesh> mBaseMesh;
    std::shared_ptr<ImmutableSkeleton> mSkeleton;

public:
    NearestJointSkinnedMesh(
            std::shared_ptr<IMesh> baseMesh,
            std::shared_ptr<ImmutableSkeleton> skeleton);

    VertexFormat GetVertexFormat() const override;

    std::size_t GetMaxVertexBufferSize() const override;
    std::size_t GetMaxIndexBufferSize() const override;

    std::size_t WriteVertices(void* buffer) const override;
    std::size_t WriteIndices(void* buffer) const override;
};

} // end namespace ng

#endif // NG_NEARESTJOINTSKINNEDMESH_HPP
