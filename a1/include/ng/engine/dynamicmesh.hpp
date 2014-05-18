#ifndef NG_DYNAMICMESH_HPP
#define NG_DYNAMICMESH_HPP

#include "ng/engine/memory.hpp"

namespace ng
{

struct VertexFormat;

class IDynamicMesh
{
public:
    virtual ~IDynamicMesh() = default;

    virtual void Init(const VertexFormat& format,
                      unique_deleted_ptr<const void> vertexData,
                      size_t vertexDataSize,
                      unique_deleted_ptr<const void> indexData,
                      size_t elementDataSize) = 0;
};

} // end namespace ng

#endif // NG_DYNAMICMESH_HPP
