#ifndef NG_SCENEGRAPH_HPP
#define NG_SCENEGRAPH_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <memory>
#include <vector>
#include <chrono>
#include <map>

namespace ng
{

class IRenderObject;
class IShaderProgram;
class RenderState;
class RenderObjectNode;
class CameraNode;
class LightNode;

class SceneGraph
{
    std::shared_ptr<RenderObjectNode> mRoot;
    std::shared_ptr<CameraNode> mCamera;
    std::vector<std::weak_ptr<LightNode>> mLights;

public:
    void SetRoot(std::shared_ptr<RenderObjectNode> root);

    void SetCamera(std::shared_ptr<CameraNode> camera);

    void AddLight(std::weak_ptr<LightNode> light);

    // Not allowed to modify the scene graph structure during an update pass.
    void Update(std::chrono::milliseconds deltaTime);

    // Not allowed to modify the scene graph during a draw pass.
    // multi-pass rendering algorithm:
    //
    // for each node
    //     for each light
    //         node.draw();
    void DrawMultiPass(const std::shared_ptr<IShaderProgram>& program,
                       const RenderState& renderState) const;
};

} // end namespace ng

#endif // NG_SCENEGRAPH_HPP
