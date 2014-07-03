#ifndef NG_RENDERBATCH_HPP
#define NG_RENDERBATCH_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <memory>
#include <vector>

namespace ng
{

class IMesh;
class SceneGraph;

class RenderObject
{
public:
    std::shared_ptr<IMesh> Mesh;
    mat4 WorldTransform;
};

class RenderBatch
{
public:
    std::vector<RenderObject> RenderObjects;

    static RenderBatch FromScene(const SceneGraph& scene);
};

} // end namespace ng

#endif // NG_RENDERBATCH_HPP
