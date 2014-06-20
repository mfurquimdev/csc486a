#include "ng/framework/scenegraph/isosurface.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/staticmesh.hpp"
#include "ng/engine/util/debug.hpp"

#include <queue>
#include <map>

namespace ng
{

IsoSurface::IsoSurface(std::shared_ptr<IRenderer> renderer)
    : mMesh(renderer->CreateStaticMesh())
{ }

void IsoSurface::Polygonize(std::function<float(vec3)> distanceFunction,
                std::function<float(float)> falloffFilterFunction,
                float isoValue,
                float voxelSize)
{
    std::function<float(vec3)> fieldFunction = [=](vec3 v){ return falloffFilterFunction(distanceFunction(v)); };

    mBoundingBox = AxisAlignedBoundingBox<float>();

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

    std::map<ivec3, std::uint8_t,std::function<bool(ivec3,ivec3)>> voxelSignMap(
    [](ivec3 a, ivec3 b){
        return std::lexicographical_compare(begin(a),end(a),begin(b),end(b));
    });

    std::vector<vec3> vertexBuffer;

    while (!toVisit.empty())
    {
        ivec3 vertexToVisit = toVisit.front();
        toVisit.pop();

        DebugPrintf("Visiting {%5d,%5d,%5d}\n", vertexToVisit.x, vertexToVisit.y, vertexToVisit.z);

        // calculate field values at all vertices
        //
        //        4----------5
        //       /|         /|
        //      6----------7 |
        //      | |        | |
        //      | |        | |
        //      | 0--------|-1
        //      |/         |/
        //      2----------3
        //
        // minExtent = vertex 0
        // maxExtent = vertex 7
        //
        // 0 1 2 3 4 5 6 7
        // | | | | | | | |
        // a b c d e f g h
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

        vec3 minExtent = vec3(vertexToVisit) * voxelSize;
        vec3 maxExtent = vec3(vertexToVisit + ivec3(1)) * voxelSize;

        vec3 a = minExtent;
        vec3 b = minExtent + vec3(voxelSize,0,0);
        vec3 c = minExtent + vec3(0,0,voxelSize);
        vec3 d = minExtent + vec3(voxelSize,0,voxelSize);
        vec3 e = maxExtent - vec3(voxelSize,0,voxelSize);
        vec3 f = maxExtent - vec3(0,0,voxelSize);
        vec3 g = maxExtent - vec3(voxelSize,0,0);
        vec3 h = maxExtent;

        std::uint8_t voxelSigns =
            (fieldFunction(a) >= isoValue) << 0 |
            (fieldFunction(b) >= isoValue) << 1 |
            (fieldFunction(c) >= isoValue) << 2 |
            (fieldFunction(d) >= isoValue) << 3 |
            (fieldFunction(e) >= isoValue) << 4 |
            (fieldFunction(f) >= isoValue) << 5 |
            (fieldFunction(g) >= isoValue) << 6 |
            (fieldFunction(h) >= isoValue) << 7 ;

        DebugPrintf("Voxel signs: %x\n", voxelSigns);

        voxelSignMap[vertexToVisit] = voxelSigns;

        // add polygons
        if (voxelSigns)
        {
            mBoundingBox.AddPoint(minExtent);
            mBoundingBox.AddPoint(maxExtent);

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

        // queue bottom voxel
        if ((voxelSigns & 0x0F) != 0x0F && (voxelSigns & 0x0F) != 0x00)
        {
            ivec3 toQueue = vertexToVisit - ivec3(0,1,0);
            if (voxelSignMap.find(toQueue) == voxelSignMap.end())
            {
                voxelSignMap[toQueue] = 0;
                toVisit.push(toQueue);
            }
        }

        // queue top voxel
        if ((voxelSigns & 0xF0) != 0xF0 && (voxelSigns & 0xF0) != 0x00)
        {
            ivec3 toQueue = vertexToVisit + ivec3(0,1,0);
            if (voxelSignMap.find(toQueue) == voxelSignMap.end())
            {
                voxelSignMap[toQueue] = 0;
                toVisit.push(toQueue);
            }
        }

        // queue left voxel
        if ((voxelSigns & 0x55) != 0x55 && (voxelSigns & 0x55) != 0x00)
        {
            ivec3 toQueue = vertexToVisit - ivec3(1,0,0);
            if (voxelSignMap.find(toQueue) == voxelSignMap.end())
            {
                voxelSignMap[toQueue] = 0;
                toVisit.push(toQueue);
            }
        }

        // queue right voxel
        if ((voxelSigns & 0x99) != 0x99 && (voxelSigns & 0x99) != 0x00)
        {
            ivec3 toQueue = vertexToVisit + ivec3(1,0,0);
            if (voxelSignMap.find(toQueue) == voxelSignMap.end())
            {
                voxelSignMap[toQueue] = 0;
                toVisit.push(toQueue);
            }
        }

        // queue back voxel
        if ((voxelSigns & 0x33) != 0x33 && (voxelSigns & 0x33) != 0x00)
        {
            ivec3 toQueue = vertexToVisit - ivec3(0,0,1);
            if (voxelSignMap.find(toQueue) == voxelSignMap.end())
            {
                voxelSignMap[toQueue] = 0;
                toVisit.push(toQueue);
            }
        }

        // queue front voxel
        if ((voxelSigns & 0xCC) != 0xCC && (voxelSigns & 0xCC) != 0x00)
        {
            ivec3 toQueue = vertexToVisit + ivec3(0,0,1);
            if (voxelSignMap.find(toQueue) == voxelSignMap.end())
            {
                voxelSignMap[toQueue] = 0;
                toVisit.push(toQueue);
            }
        }
    }

    std::size_t numVertices = vertexBuffer.size() / 2;
    DebugPrintf("Got %zu vertices\n", numVertices);
    if (numVertices == 0)
    {
        return;
    }

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
