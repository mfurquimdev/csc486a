#include "ng/engine/rendering/renderbatch.hpp"

#include "ng/engine/rendering/scenegraph.hpp"

#include "ng/engine/util/scopeguard.hpp"

namespace ng
{

void GetRenderObjects(
        std::vector<RenderObject>& ROs,
        std::vector<mat4>& mvstack,
        const SceneGraphNode& node)
{
    mvstack.push_back(mvstack.back() * node.Transform);
    auto mvstackScope = make_scope_guard([&]{
        mvstack.pop_back();
    });

    ROs.push_back(RenderObject{node.Mesh,mvstack.back()});

    for (const std::shared_ptr<SceneGraphNode>& child : node.Children)
    {
        if (child == nullptr)
        {
            continue;
        }

        GetRenderObjects(ROs, mvstack, *child);
    }
}

RenderBatch RenderBatch::FromScene(const SceneGraph& scene)
{
    // start with identity
    std::vector<mat4> mvstack{mat4()};

    RenderBatch batch;

    if (scene.Root != nullptr)
    {
        GetRenderObjects(batch.RenderObjects, mvstack, *scene.Root);
    }

    return std::move(batch);
}

} // end namespace ng
