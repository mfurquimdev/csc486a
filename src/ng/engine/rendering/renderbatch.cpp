#include "ng/engine/rendering/renderbatch.hpp"

#include "ng/engine/rendering/scenegraph.hpp"

#include "ng/engine/util/scopeguard.hpp"

namespace ng
{

static void ConvertToRenderBatch(
        const std::shared_ptr<const SceneGraphNode>& node,
        const std::vector<std::shared_ptr<SceneGraphCameraNode>>& cameras,
        std::vector<mat4>& mvstack,
        std::vector<RenderObject>& renderObjects,
        std::vector<RenderCamera>& renderCameras)
{
    if (node == nullptr)
    {
        // not the base case of recursion,
        // just an extra precaution.
        return;
    }

    mvstack.push_back(mvstack.back() * node->Transform);
    auto mvstackScope = make_scope_guard([&]{
        mvstack.pop_back();
    });

    if (node->Mesh != nullptr)
    {
        renderObjects.push_back(
                    RenderObject{
                        node->Mesh,
                        node->Material,
                        mvstack.back()});
    }

    // check if we're visiting a camera
    auto cameraIt = std::find(
            cameras.begin(),
            cameras.end(),
            node);

    if (cameraIt != cameras.end() &&
       *cameraIt != nullptr)
    {
        const SceneGraphCameraNode& cam = **cameraIt;

        renderCameras.push_back(
                    RenderCamera{
                        cam.Projection,
                        inverse(cam.Transform),
                        cam.ViewportTopLeft,
                        cam.ViewportSize});
    }

    for (const std::shared_ptr<const SceneGraphNode>& child : node->Children)
    {
        if (child == nullptr)
        {
            continue;
        }

        ConvertToRenderBatch(
                    child,
                    cameras,
                    mvstack,
                    renderObjects,
                    renderCameras);
    }
}

RenderBatch RenderBatch::FromScene(const SceneGraph& scene)
{
    // start with identity
    std::vector<mat4> mvstack{mat4()};

    RenderBatch batch;

    // get the 3D scene
    ConvertToRenderBatch(
                scene.Root,
                scene.ActiveCameras,
                mvstack,
                batch.RenderObjects,
                batch.RenderCameras);

    // get the 2D overlay scene
    ConvertToRenderBatch(
                scene.OverlayRoot,
                scene.OverlayActiveCameras,
                mvstack,
                batch.OverlayRenderObjects,
                batch.OverlayRenderCameras);

    return std::move(batch);
}

} // end namespace ng
