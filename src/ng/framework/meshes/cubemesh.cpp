#include "ng/framework/meshes/cubemesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"

#include <cstring>
#include <cstddef>

namespace ng
{

CubeMesh::CubeMesh(float sideLength)
    : mSideLength(sideLength)
{ }

VertexFormat CubeMesh::GetVertexFormat() const
{
    VertexFormat fmt;

    fmt.Position = VertexAttribute(
                3,
                ArithmeticType::Float,
                false,
                sizeof(CubeMesh::Vertex),
                offsetof(CubeMesh::Vertex, Position));

    fmt.Normal = VertexAttribute(
                3,
                ArithmeticType::Float,
                false,
                sizeof(CubeMesh::Vertex),
                offsetof(CubeMesh::Vertex, Normal));

    return fmt;
}

std::size_t CubeMesh::GetMaxVertexBufferSize() const
{
    constexpr int NumFaces = 6;
    constexpr int TrianglesPerFace = 2;
    constexpr int VerticesPerTriangle = 3;
    constexpr std::size_t SizeOfVertex = sizeof(CubeMesh::Vertex);
    return NumFaces * TrianglesPerFace * VerticesPerTriangle * SizeOfVertex;
}

std::size_t CubeMesh::GetMaxIndexBufferSize() const
{
    return 0;
}

std::size_t CubeMesh::WriteVertices(void* buffer) const
{
    if (buffer)
    {
        //   f----------g
        //  /|         /|
        // e----------h |
        // | |        | |
        // | |        | |
        // | a--------|-b
        // |/         |/
        // d----------c
        //
        //              -z
        //               ^
        //         +y   /
        //          ^  /
        //          | /
        //    -x <-- --> +x
        //         /|
        //        / v
        //       / -y
        //      v
        //     +z

        // positions
        const vec3 minExtent = vec3(-mSideLength / 2);
        const vec3 maxExtent = vec3( mSideLength / 2);

        const vec3 a = minExtent;
        const vec3 b = minExtent + vec3(mSideLength,0,0);
        const vec3 c = minExtent + vec3(mSideLength, 0, mSideLength);
        const vec3 d = minExtent + vec3(0,0,mSideLength);
        const vec3 e = maxExtent - vec3(mSideLength,0,0);
        const vec3 f = maxExtent - vec3(mSideLength,0,mSideLength);
        const vec3 g = maxExtent - vec3(0,0,mSideLength);
        const vec3 h = maxExtent;

        const vec3    top( 0, 1, 0);
        const vec3 bottom( 0,-1, 0);
        const vec3   left(-1, 0, 0);
        const vec3  right( 1, 0, 0);
        const vec3  front( 0, 0, 1);
        const vec3   back( 0, 0,-1);

        const CubeMesh::Vertex vertexData[][3] = {
            { { a, bottom }, { b, bottom }, { c, bottom } }, // bottom tri 1
            { { c, bottom }, { d, bottom }, { a, bottom } }, // bottom tri 2
            { { d, front  }, { c, front  }, { h, front  } }, // front tri 1
            { { h, front  }, { e, front  }, { d, front  } }, // front tri 2
            { { a, left   }, { d, left   }, { e, left   } }, // left tri 1
            { { e, left   }, { f, left   }, { a, left   } }, // left tri 2
            { { b, back   }, { a, back   }, { f, back   } }, // back tri 1
            { { f, back   }, { g, back   }, { b, back   } }, // back tri 2
            { { c, right  }, { b, right  }, { g, right  } }, // right tri 1
            { { g, right  }, { h, right  }, { c, right  } }, // right tri 2
            { { h, top    }, { g, top    }, { f, top    } }, // top tri 1
            { { f, top    }, { e, top    }, { h, top    } }  // top tri 2
        };

        std::memcpy(buffer, vertexData, sizeof(vertexData));
    }

    return 36;
}

std::size_t CubeMesh::WriteIndices(void*) const
{
    return 0;
}

} // end namespace ng
