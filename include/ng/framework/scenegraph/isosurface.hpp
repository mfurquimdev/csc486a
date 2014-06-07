#ifndef NG_ISOSURFACE_HPP
#define NG_ISOSURFACE_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <memory>
#include <functional>

namespace ng
{

class IRenderer;

class IsoSurface
{
    std::shared_ptr<IRenderer> mRenderer;

public:
    IsoSurface(std::shared_ptr<IRenderer> renderer);

    void Polygonize(std::function<float(vec3)> fieldFunction,
                    float isoValue,
                    float voxelSize,
                    std::vector<ivec3> seedVoxels);
};

} // end namespace ng

#endif // NG_ISOSURFACE_HPP
