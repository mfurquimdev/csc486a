#include "ng/framework/scenegraph/isosurface.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/staticmesh.hpp"
#include "ng/engine/util/debug.hpp"
#include "ng/engine/util/profiler.hpp"

#include <unordered_map>
#include <array>
#include <queue>

namespace ng
{

IsoSurface::IsoSurface(std::shared_ptr<IRenderer> renderer)
    : mMesh(renderer->CreateStaticMesh())
{ }

template<unsigned int Nbits=5>
class WyvillHash
{
public:
    static constexpr unsigned int NBits = Nbits;
    static constexpr unsigned int BMask = (1 << NBits) - 1;

    constexpr std::uint16_t operator()(ivec3 i)
    {
        // takes 5 least significant bits of x,y,z and combines them
        // result: 0xxxxxyyyyyzzzzz
        return ((((i.x & BMask) << NBits) | (i.y & BMask)) << NBits) | (i.z & BMask);
    }
};

void IsoSurface::Polygonize(std::function<float(vec3)> distanceFunction,
                std::function<float(float)> falloffFilterFunction,
                float isoValue,
                float voxelSize)
{
    Profiler polygonizeTimer;
    polygonizeTimer.Start();

    struct TableEntry
    {
        float Field;
        bool  Visited;
    };

    std::unordered_map<ivec3, TableEntry, WyvillHash<>> hashTable;
    hashTable.reserve(1 << (WyvillHash<>::NBits * 3));

    std::function<float(vec3)> fieldFunction = [&](vec3 v){ return falloffFilterFunction(distanceFunction(v)); };

    AxisAlignedBoundingBox<float> bbox;

    // queue of nodes to visit
    std::queue<ivec3> toVisit;

    // search for initial seed node to visit
    ivec3 seed(0,0,0);
    while (fieldFunction(vec3(seed) * voxelSize) >= isoValue)
    {
        seed.x++;
    }
    if (seed.x > 0) seed.x--;
    toVisit.push(seed);

    // mapping of indices to voxel vertices
    //
    //        2----------6
    //       /|         /|
    //      3----------7 |
    //      | |        | |
    //      | |        | |
    //      | 0--------|-4
    //      |/         |/
    //      1----------5
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

    const std::array<ivec3,8> kDirections {
        ivec3{ 0, 0, 0 }, // 0
        ivec3{ 0, 0, 1 }, // 1
        ivec3{ 0, 1, 0 }, // 2
        ivec3{ 0, 1, 1 }, // 3
        ivec3{ 1, 0, 0 }, // 4
        ivec3{ 1, 0, 1 }, // 5
        ivec3{ 1, 1, 0 }, // 6
        ivec3{ 1, 1, 1 }  // 7
    };

    // the bits that correspond to that face and the direction of the face
    const std::array<std::pair<std::uint8_t,ivec3>,8> kFaces {
        std::pair<std::uint8_t,ivec3>{ 0x0F, ivec3{ -1,  0,  0 } }, // left
        std::pair<std::uint8_t,ivec3>{ 0xF0, ivec3{ +1,  0,  0 } }, // right
        std::pair<std::uint8_t,ivec3>{ 0x33, ivec3{  0, -1,  0 } }, // bottom
        std::pair<std::uint8_t,ivec3>{ 0xCC, ivec3{  0, +1,  0 } }, // top
        std::pair<std::uint8_t,ivec3>{ 0x55, ivec3{  0,  0, -1 } }, // back
        std::pair<std::uint8_t,ivec3>{ 0xAA, ivec3{ -1,  0, +1 } }, // front
    };

    std::vector<vec3> vertexBuffer;

    while (!toVisit.empty())
    {
        ivec3 vertexToVisit = toVisit.front();
        toVisit.pop();

        {
            // check if it was already visited
            auto it = hashTable.find(vertexToVisit);
            if (it != hashTable.end() && it->second.Visited)
            {
                continue;
            }
        }

        // calculate the absolute values of the lattice indices
        std::array<ivec3,8> latticeIndices;
        for (std::size_t i = 0; i < latticeIndices.size(); i++)
        {
            latticeIndices[i] = vertexToVisit + kDirections[i];
        }

        // calculate the field value at all indices or grab the cached ones.
        std::array<float,8> fieldValues;
        for (std::size_t i = 0; i < fieldValues.size(); i++)
        {
            ivec3 latticeIndex = latticeIndices[i];
            auto it = hashTable.find(latticeIndex);
            if (it != hashTable.end())
            {
                fieldValues[i] = it->second.Field;
            }
            else
            {
                fieldValues[i] = fieldFunction(vec3(latticeIndex) * voxelSize);
                hashTable.emplace(latticeIndex, TableEntry{ fieldValues[i], false });
            }
        }

        // calculate the signs of all corners
        // inside  = 1 / '+' / f >= iso
        // outside = 0 / '-' / f <  iso
        std::uint8_t signBits = 0;
        for (std::size_t i = 0; i < fieldValues.size(); i++)
        {
            signBits |= (fieldValues[i] >= isoValue) << i;
        }

        hashTable[vertexToVisit].Visited = true;

        // add polygons
        if (signBits)
        {
            vec3 minExtent = vec3(vertexToVisit) * voxelSize;
            vec3 maxExtent = vec3(vertexToVisit + ivec3(1)) * voxelSize;

            // the order here doesn't match the cube in the comment up there,
            // but it's placeholder so whatever.
            vec3 a = minExtent;
            vec3 b = minExtent + vec3(voxelSize,0,0);
            vec3 c = minExtent + vec3(0,0,voxelSize);
            vec3 d = minExtent + vec3(voxelSize,0,voxelSize);
            vec3 e = maxExtent - vec3(voxelSize,0,voxelSize);
            vec3 f = maxExtent - vec3(0,0,voxelSize);
            vec3 g = maxExtent - vec3(voxelSize,0,0);
            vec3 h = maxExtent;

            bbox.AddPoint(minExtent);
            bbox.AddPoint(maxExtent);

            // normals
            vec3 top(0,1,0);
            vec3 bottom(0,-1,0);
            vec3 left(-1,0,0);
            vec3 right(1,0,0);
            vec3 front(0,0,1);
            vec3 back(0,0,-1);

            vertexBuffer.insert(vertexBuffer.end(), {
                a,bottom, b,bottom, c,bottom,   b,bottom, d,bottom, c,bottom,   // bottom face
                c,front,  d,front,  g,front,    d,front,  h,front,  g,front,    // front face
                a,left,   c,left,   e,left,     c,left,   g,left,   e,left,     // left face
                b,back,   a,back,   f,back,     a,back,   e,back,   f,back,     // back face
                d,right,  b,right,  h,right,    b,right,  f,right,  h,right,    // right face
                g,top,    h,top,    e,top,      h,top,    f,top,    e,top       // top face
            });
        }

        // add neighbours if their faces intersect with the surface of the field
        for (std::size_t i = 0; i < kFaces.size(); i++)
        {
            std::uint8_t bits = kFaces[i].first;
            if ((signBits & bits) != bits && (signBits & bits) != 0x00)
            {
                ivec3 toQueue = vertexToVisit + kFaces[i].second;
                auto it = hashTable.find(toQueue);
                if (it == hashTable.end() || !it->second.Visited)
                {
                    toVisit.push(toQueue);
                }
            }
        }
    }

    polygonizeTimer.Stop();
    DebugPrintf("Time to polygonize: %ums\n", polygonizeTimer.GetTotalTimeMS());

    std::size_t numVertices = vertexBuffer.size() / 2;
    if (numVertices > 0)
    {
        VertexFormat meshFormat({
                { VertexAttributeName::Position, VertexAttribute(3, ArithmeticType::Float, false, 2 * sizeof(vec3), 0) },
                { VertexAttributeName::Normal,   VertexAttribute(3, ArithmeticType::Float, false, 2 * sizeof(vec3), sizeof(vec3)) }
            });

        std::shared_ptr<std::vector<vec3>> pVertexBuffer(new std::vector<vec3>(std::move(vertexBuffer)));

        std::pair<std::shared_ptr<const void>,std::ptrdiff_t> pBufferData({pVertexBuffer->data(), [pVertexBuffer](const void*){}},
                                                                          numVertices * 2 * sizeof(vec3));

        mMesh->Init(meshFormat, {
                        { VertexAttributeName::Position, pBufferData },
                        { VertexAttributeName::Normal,   pBufferData }
                    }, nullptr, 0, numVertices);
    }
    else
    {
        mMesh->Reset();
    }

    mBoundingBox = std::move(bbox);
}

RenderObjectPass IsoSurface::Draw(
        const std::shared_ptr<IShaderProgram>& program,
        const std::map<std::string, UniformValue>& uniforms,
        const RenderState& renderState)
{
    mMesh->Draw(program, uniforms, renderState,
                PrimitiveType::Triangles, 0, mMesh->GetVertexCount());
    return RenderObjectPass::Continue;
}

} // end namespace ng
