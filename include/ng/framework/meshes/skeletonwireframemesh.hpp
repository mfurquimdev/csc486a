#ifndef NG_SKELETONWIREFRAMEMESH_HPP
#define NG_SKELETONWIREFRAMEMESH_HPP

#include "ng/engine/rendering/mesh.hpp"
#include "ng/engine/util/immutable.hpp"

#include <memory>

namespace ng
{

class Skeleton;
class SkinningMatrixPalette;

class SkeletonWireframeMesh : public IMesh
{
    class Vertex;

    const std::shared_ptr<immutable<Skeleton>> mSkeleton;
    const std::shared_ptr<immutable<SkinningMatrixPalette>> mPalette;


public:
    SkeletonWireframeMesh(
        std::shared_ptr<immutable<Skeleton>> skeleton,
        std::shared_ptr<immutable<SkinningMatrixPalette>> palette);

    VertexFormat GetVertexFormat() const override;

    std::size_t GetMaxVertexBufferSize() const override;
    std::size_t GetMaxIndexBufferSize() const override;

    std::size_t WriteVertices(void* buffer) const override;
    std::size_t WriteIndices(void* buffer) const override;
};

} // end namespace ng

#endif // NG_SKELETONWIREFRAMEMESH_HPP
