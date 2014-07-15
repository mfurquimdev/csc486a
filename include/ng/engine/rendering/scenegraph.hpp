#ifndef NG_SCENEGRAPH_HPP
#define NG_SCENEGRAPH_HPP

#include "ng/engine/rendering/material.hpp"

#include "ng/engine/math/linearalgebra.hpp"

#include <vector>
#include <memory>

namespace ng
{

class IMesh;

class SceneGraphNode
{
public:
    mat4 Transform;
    std::shared_ptr<IMesh> Mesh;
    ng::Material Material;
    std::vector<std::shared_ptr<SceneGraphNode>> Children;
};

class SceneGraphCameraNode : public SceneGraphNode
{
public:
    mat4 Projection;

    ivec2 ViewportTopLeft;
    ivec2 ViewportSize;
};

class SceneGraph
{
public:
    std::shared_ptr<SceneGraphNode> Root;
    std::vector<std::shared_ptr<SceneGraphCameraNode>> ActiveCameras;

    std::shared_ptr<SceneGraphNode> OverlayRoot;
    std::vector<std::shared_ptr<SceneGraphCameraNode>> OverlayActiveCameras;
};

} // end namespace ng

#endif // NG_SCENEGRAPH_HPP
