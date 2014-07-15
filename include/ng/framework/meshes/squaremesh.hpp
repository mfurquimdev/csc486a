#ifndef NG_SQUAREMESH_HPP
#define NG_SQUAREMESH_HPP

#include "ng/engine/rendering/mesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"

namespace ng
{

class SquareMesh : public IMesh
{
    const float mSideLength;

public:
    class Vertex
    {
    public:
        vec2 Position;
        vec2 Texcoord;
        vec3 Normal;
    };

    SquareMesh(float sideLength);

    VertexFormat GetVertexFormat() const override;

    std::size_t GetMaxVertexBufferSize() const override;
    std::size_t GetMaxIndexBufferSize() const override;

    std::size_t WriteVertices(void* buffer) const override;
    std::size_t WriteIndices(void* buffer) const override;
};

} // end namespace ng

#endif // NG_SQUAREMESH_HPP
