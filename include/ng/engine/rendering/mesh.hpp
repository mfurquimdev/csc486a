#ifndef NG_MESH_HPP
#define NG_MESH_HPP

#include "ng/engine/rendering/vertexformat.hpp"

#include <cstddef>

namespace ng
{

class IMesh
{
public:
    virtual ~IMesh() = default;

    // must be consistent with what is written.
    virtual VertexFormat GetVertexFormat() const = 0;

    // upper bounds used to reserve memory.
    virtual std::size_t GetMaxVertexBufferSize() const = 0;
    virtual std::size_t GetMaxIndexBufferSize() const = 0;

    // return the number of vertices/indices actually written.
    virtual std::size_t WriteVertices(void* buffer) const = 0;
    virtual std::size_t WriteIndices(void* buffer) const = 0;
};

} // end namespace ng

#endif // NG_MESH_HPP
