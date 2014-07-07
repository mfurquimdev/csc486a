#ifndef NG_CUBEMESH_HPP
#define NG_CUBEMESH_HPP

#include "ng/engine/rendering/mesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"

namespace ng
{

class CubeMesh : public IMesh
{
    float mSideLength;

public:
    struct Vertex
    {
        vec3 Position;
        vec3 Normal;
    };

    CubeMesh(float sideLength);

    VertexFormat GetVertexFormat() const override;

    std::size_t GetMaxVertexBufferSize() const override;
    std::size_t GetMaxElementBufferSize() const override;

    std::size_t WriteVertices(void* buffer) const override;
    std::size_t WriteIndices(void* buffer) const override;
};

} // end namespace ng

#endif // NG_CUBEMESH_HPP
