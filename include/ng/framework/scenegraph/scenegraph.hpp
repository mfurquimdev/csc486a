#ifndef NG_SCENEGRAPH_HPP
#define NG_SCENEGRAPH_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <memory>
#include <vector>
#include <chrono>

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
    std::shared_ptr<RenderObjectNode> mUpdateRoot;
    std::vector<std::shared_ptr<CameraNode>> mCameras;
    std::vector<std::shared_ptr<LightNode>> mLights;

public:
    void SetUpdateRoot(std::shared_ptr<RenderObjectNode> updateRoot);

    void AddCamera(std::shared_ptr<CameraNode> currentCamera);

    void AddLight(std::shared_ptr<LightNode> light);

    // Currently not allowed to modify the scene graph during an update pass.
    // ... but that might be a nice feature for later? Just gotta find a good way to do it.
    void Update(std::chrono::milliseconds deltaTime);

    // Not allowed to modify the scene graph during a draw pass.
    // multi-pass rendering algorithm:
    //
    // for each camera
    //     for each node
    //         for each light
    //             node.draw();
    void DrawMultiPass(const std::shared_ptr<IShaderProgram>& program,
                       const RenderState& renderState) const;
};

} // end namespace ng

#endif // NG_SCENEGRAPH_HPP
