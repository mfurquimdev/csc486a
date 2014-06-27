#ifndef NG_RENDERBATCH_HPP
#define NG_RENDERBATCH_HPP

#include "ng/engine/rendering/renderer.hpp"

#include <cstdint>
#include <memory>

namespace ng
{

class MeshInstanceID
{
public:
    const std::uint32_t ID;
};

class IRenderBatch
{
public:
    virtual ~IRenderBatch() = default;

    virtual void Commit() = 0;

    virtual void SetShouldSwapBuffers(bool shouldSwapBuffers) = 0;
    virtual bool GetShouldSwapBuffers() const = 0;

    virtual MeshInstanceID AddMeshInstance(MeshID meshID) = 0;
    virtual void RemoveMeshInstance(MeshInstanceID meshInstanceID) = 0;
};

} // end namespace ng

#endif // NG_RENDERBATCH_HPP
