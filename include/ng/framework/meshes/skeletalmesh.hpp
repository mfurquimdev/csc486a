#ifndef NG_SKELETALMESH_HPP
#define NG_SKELETALMESH_HPP

#include "ng/engine/rendering/mesh.hpp"
#include "ng/framework/loaders/objloader.hpp"
#include "ng/framework/models/skeletalmodel.hpp"

namespace ng
{

class SkeletalMesh : public IMesh
{
public:
    VertexFormat GetVertexFormat() const override;

    std::size_t GetMaxVertexBufferSize() const override;
    std::size_t GetMaxIndexBufferSize() const override;

    std::size_t WriteVertices(void* buffer) const override;
    std::size_t WriteIndices(void* buffer) const override;
};

} // end namespace ng

#endif // NG_SKELETALMESH_HPP
