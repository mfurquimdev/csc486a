#ifndef NG_RENDERBATCH_HPP
#define NG_RENDERBATCH_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <memory>
#include <vector>

namespace ng
{

class IMesh;
class Material;
class SceneGraph;

class RenderObject
{
public:
    std::shared_ptr<IMesh> Mesh;
    std::shared_ptr<ng::Material> Material;
    mat4 WorldTransform;
};

class RenderCamera
{
public:
    mat4 Projection;
    mat4 WorldView;

    ivec2 ViewportTopLeft;
    ivec2 ViewportSize;
};

class RenderBatch
{
public:
    static RenderBatch FromScene(const SceneGraph& scene);

    std::vector<RenderObject> RenderObjects;
    std::vector<RenderCamera> RenderCameras;

    std::vector<RenderObject> OverlayRenderObjects;
    std::vector<RenderCamera> OverlayRenderCameras;
};

} // end namespace ng

#endif // NG_RENDERBATCH_HPP
