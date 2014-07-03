#include "ng/engine/rendering/renderbatch.hpp"

#include "ng/engine/rendering/scenegraph.hpp"

#include "ng/engine/util/scopeguard.hpp"

namespace ng
{

static void ConvertToRenderBatch(
        const SceneGraph& graph,
        const std::shared_ptr<const SceneGraphNode>& node,
        std::vector<mat4>& mvstack,
        RenderBatch& batch)
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
        batch.RenderObjects.push_back(
                RenderObject{
                    node->Mesh,
                    mvstack.back()});
    }

    // check if we're visiting a camera
    auto cameraIt = std::find(
            graph.ActiveCameras.begin(),
            graph.ActiveCameras.end(),
            node);

    if (cameraIt != graph.ActiveCameras.end() &&
       *cameraIt != nullptr)
    {
        const SceneGraphCameraNode& cam = **cameraIt;
        batch.RenderCameras.push_back(
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

        ConvertToRenderBatch(graph, child, mvstack, batch);
    }
}

RenderBatch RenderBatch::FromScene(const SceneGraph& scene)
{
    // start with identity
    std::vector<mat4> mvstack{mat4()};

    RenderBatch batch;

    ConvertToRenderBatch(
            scene,
            scene.Root,
            mvstack,
            batch);

    return std::move(batch);
}

} // end namespace ng
