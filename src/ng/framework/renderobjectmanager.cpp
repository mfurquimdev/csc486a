#include "ng/framework/renderobjectmanager.hpp"

#include "ng/engine/uniform.hpp"

#include "ng/framework/renderobject.hpp"
#include "ng/framework/renderobjectnode.hpp"

#include <stack>
#include <algorithm>

namespace ng
{

std::shared_ptr<RenderObjectNode> RenderObjectManager::AddRoot()
{
    mRootNodes.push_back(std::make_shared<RenderObjectNode>());
    return mRootNodes.back();
}

void RenderObjectManager::RemoveRoot(std::shared_ptr<RenderObjectNode> node)
{
    auto it = std::find(mRootNodes.begin(), mRootNodes.end(), node);

    if (it == mRootNodes.end())
    {
        throw std::logic_error("Cannot remove a non-root from the root set");
    }

    mRootNodes.erase(it);
}

static void UpdateDepthFirst(
        std::chrono::milliseconds deltaTime,
        const std::shared_ptr<RenderObjectNode>& node)
{
    if (node)
    {
        RenderObjectPass pass = RenderObjectPass::Continue;

        if (node->GetRenderObject())
        {
            pass = node->GetRenderObject()->PreUpdate(deltaTime, *node);
        }

        if (pass != RenderObjectPass::SkipChildren)
        {
            for (const std::shared_ptr<RenderObjectNode>& child : node->GetChildren())
            {
                UpdateDepthFirst(deltaTime, child);
            }
        }

        if (node->GetRenderObject())
        {
            node->GetRenderObject()->PostUpdate(deltaTime, *node);
        }
    }
}

void RenderObjectManager::Update(std::chrono::milliseconds deltaTime)
{
    for (const std::shared_ptr<RenderObjectNode>& root : GetRoots())
    {
        UpdateDepthFirst(deltaTime, root);
    }
}

using MatrixStack = std::stack<mat4, std::vector<mat4>>;

static void DrawDepthFirst(
        MatrixStack& projectionStack,
        MatrixStack& modelViewStack,
        const std::shared_ptr<IShaderProgram>& program,
        const RenderState& renderState,
        const std::shared_ptr<RenderObjectNode>& node)
{
    if (node)
    {
        struct MatrixStackScope
        {
            MatrixStack& mMatStack;

            MatrixStackScope(MatrixStack& matStack, mat4 m)
                : mMatStack(matStack)
            {
                mat4 newtop = mMatStack.top() * m;
                mMatStack.push(newtop);
            }

            ~MatrixStackScope()
            {
                mMatStack.pop();
            }
        };

        MatrixStackScope projectionScope(projectionStack, node->GetProjectionTransform());
        MatrixStackScope modelViewScope(modelViewStack, node->GetModelViewTransform());

        if (node->GetRenderObject())
        {
            mat4 projection = projectionStack.top();
            mat4 modelView = modelViewStack.top();

            node->GetRenderObject()->Draw(program, {
                                       { "uProjection", projection },
                                       { "uModelView", modelView }
                                   }, renderState);
        }

        for (const std::shared_ptr<RenderObjectNode>& child : node->GetChildren())
        {
            DrawDepthFirst(projectionStack, modelViewStack, program, renderState, child);
        }
    }
}

void RenderObjectManager::Draw(
        const std::shared_ptr<IShaderProgram>& program,
        const RenderState& renderState) const
{
    mat4 id = mat4();

    MatrixStack modelViewStack, projectionStack;
    modelViewStack.push(id);
    projectionStack.push(id);

    for (const std::shared_ptr<RenderObjectNode>& root : GetRoots())
    {
        DrawDepthFirst(projectionStack, modelViewStack, program, renderState, root);
    }
}

} // end namespace ng
