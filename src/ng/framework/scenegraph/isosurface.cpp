#include "ng/framework/scenegraph/isosurface.hpp"

#include "ng/engine/rendering/renderer.hpp"

#include <stack>
#include <map>

namespace ng
{

IsoSurface::IsoSurface(std::shared_ptr<IRenderer> renderer)
    : mRenderer(renderer)
{ }

void IsoSurface::Polygonize(std::function<float(vec3)> fieldFunction,
                            float isoValue,
                            float voxelSize,
                            std::vector<ivec3> seedVoxels)
{
    // add seeds to visit stack
    std::stack<ivec3> toVisit;
    for (ivec3 seed : seedVoxels)
    {
        toVisit.push(seed);
    }

    std::map<ivec3, std::uint8_t,std::function<bool(ivec3,ivec3)>> voxelSignMap(
    [](ivec3 a, ivec3 b){
        return a.x < b.x || a.y < b.y || a.z < b.z;
    });

    std::vector<vec3> vertexBuffer;

    while (!toVisit.empty())
    {
        ivec3 vertexToVisit = toVisit.top();
        toVisit.pop();

        auto it = voxelSignMap.lower_bound(vertexToVisit);
        if (it != voxelSignMap.end() || it->first == vertexToVisit)
        {
            // already visited
            continue;
        }

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

        std::uint8_t voxelSigns =
            (fieldFunction(minExtent)                               < isoValue) << 0 |
            (fieldFunction(minExtent + vec3(voxelSize,0,0))         < isoValue) << 1 |
            (fieldFunction(minExtent + vec3(0,0,voxelSize))         < isoValue) << 2 |
            (fieldFunction(minExtent + vec3(voxelSize,0,voxelSize)) < isoValue) << 3 |
            (fieldFunction(maxExtent - vec3(voxelSize,0,voxelSize)) < isoValue) << 4 |
            (fieldFunction(maxExtent - vec3(0,0,voxelSize))         < isoValue) << 5 |
            (fieldFunction(maxExtent - vec3(voxelSize,0,0))         < isoValue) << 6 |
            (fieldFunction(maxExtent)                               < isoValue) << 7 ;

        voxelSignMap.emplace_hint(it, vertexToVisit, voxelSigns);

        // add polygons

        // queue bottom voxel
        if ((voxelSigns & 0x0F) != 0x0F && (voxelSigns & 0x0F) != 0x00)
        {
            toVisit.push(vertexToVisit - ivec3(0,1,0));
        }

        // queue top voxel
        if ((voxelSigns & 0xF0) != 0xF0 && (voxelSigns & 0xF0) != 0x00)
        {
            toVisit.push(vertexToVisit + ivec3(0,1,0));
        }

        // queue left voxel
        if ((voxelSigns & 0x55) != 0x55 && (voxelSigns & 0x55) != 0x00)
        {
            toVisit.push(vertexToVisit - ivec3(1,0,0));
        }

        // queue right voxel
        if ((voxelSigns & 0x99) != 0x99 && (voxelSigns & 0x99) != 0x00)
        {
            toVisit.push(vertexToVisit + ivec3(1,0,0));
        }

        // queue back voxel
        if ((voxelSigns & 0x33) != 0x33 && (voxelSigns & 0x33) != 0x00)
        {
            toVisit.push(vertexToVisit - ivec3(0,0,1));
        }

        // queue front voxel
        if ((voxelSigns & 0xCC) != 0xCC && (voxelSigns & 0xCC) != 0x00)
        {
            toVisit.push(vertexToVisit + ivec3(0,0,1));
        }
    }
}

} // end namespace ng
