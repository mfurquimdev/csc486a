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

void IsoSurface::Polygonize(std::function<float(vec3)> fieldFunction,
                            float isoValue,
                            float voxelSize,
                            std::vector<ivec3> seedVoxels)
{
    mBoundingBox = AxisAlignedBoundingBox<float>();

    // make sure seedVoxels are unique
    std::sort(seedVoxels.begin(), seedVoxels.end(), [](ivec3 a, ivec3 b){
        return a.x < b.x || ((a.x == b.x && a.y < b.y) || (a.y == b.y && a.z < b.z));
    });

    seedVoxels.erase(std::unique(seedVoxels.begin(), seedVoxels.end()), seedVoxels.end());

    // add seeds to visit queue
    std::queue<ivec3> toVisit;

    for (ivec3 seed : seedVoxels)
    {
        toVisit.push(seed);
    }

    std::map<ivec3, std::uint8_t,std::function<bool(ivec3,ivec3)>> voxelSignMap(
    [](ivec3 a, ivec3 b){
        return a.x < b.x || ((a.x == b.x && a.y < b.y) || (a.y == b.y && a.z < b.z));
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
            (fieldFunction(a) < isoValue) << 0 |
            (fieldFunction(b) < isoValue) << 1 |
            (fieldFunction(c) < isoValue) << 2 |
            (fieldFunction(d) < isoValue) << 3 |
            (fieldFunction(e) < isoValue) << 4 |
            (fieldFunction(f) < isoValue) << 5 |
            (fieldFunction(g) < isoValue) << 6 |
            (fieldFunction(h) < isoValue) << 7 ;

        DebugPrintf("Voxel signs: %x\n", voxelSigns);

        voxelSignMap[vertexToVisit] = voxelSigns;

        // add polygons
        if (voxelSigns)
        {
            mBoundingBox.AddPoint(minExtent);
            mBoundingBox.AddPoint(maxExtent);

            vertexBuffer.insert(vertexBuffer.end(), {
                                    a,b,c, b,d,c, // bottom face
                                    c,d,g, d,h,g, // front face
                                    a,c,e, c,g,e, // left face
                                    b,a,f, a,e,f, // back face
                                    d,b,h, b,f,h, // right face
                                    g,h,e, h,f,e  // top face
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

    std::size_t numVertices = vertexBuffer.size();
    DebugPrintf("Got %zu vertices\n", numVertices);
    if (numVertices == 0)
    {
        return;
    }

    VertexFormat::AttributeMap attributes;
    attributes[VertexAttributeName::Position] = VertexAttribute(3, ArithmeticType::Float, false, 0, 0);

    std::map<VertexAttributeName,std::pair<std::shared_ptr<const void>,std::ptrdiff_t>> attributeDataAndSize;
    std::shared_ptr<std::vector<vec3>> pVertexBuffer(new std::vector<vec3>(std::move(vertexBuffer)));
    attributeDataAndSize[VertexAttributeName::Position] = std::pair<std::shared_ptr<const void>,std::ptrdiff_t>(
        std::shared_ptr<const void>(pVertexBuffer->data(), [pVertexBuffer](const void*){}),
        pVertexBuffer->size() * sizeof(vec3));

    mMesh->Init(VertexFormat(attributes), attributeDataAndSize, nullptr, 0, numVertices);
}

RenderObjectPass IsoSurface::Draw(
        const std::shared_ptr<IShaderProgram>& program,
        const std::map<std::string, UniformValue>& uniforms,
        const RenderState& renderState)
{
    std::map<std::string, UniformValue> modUniforms = uniforms;
    modUniforms.emplace("uTint", vec4(1,0,0,1));
    mMesh->Draw(program, modUniforms, renderState, PrimitiveType::Triangles, 0, mMesh->GetVertexCount());
    return RenderObjectPass::Continue;
}

} // end namespace ng
