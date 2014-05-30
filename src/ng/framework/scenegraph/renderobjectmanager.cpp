#include "ng/framework/scenegraph/scenegraph.hpp"

#include "ng/engine/rendering/uniform.hpp"

#include "ng/framework/scenegraph/camera.hpp"

#include "ng/framework/scenegraph/renderobject.hpp"
#include "ng/framework/scenegraph/renderobjectnode.hpp"

#include "ng/engine/rendering/renderstate.hpp"

#include <stack>
#include <algorithm>

namespace ng
{

void SceneGraph::SetCurrentCamera(std::shared_ptr<CameraNode> currentCamera)
{
    if (mCurrentCamera)
    {
        mCurrentCamera->GetCamera()->mIsCurrentCamera = false;
    }

    mCurrentCamera = currentCamera;
    mCurrentCamera->GetCamera()->mIsCurrentCamera = true;
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

void SceneGraph::Update(std::chrono::milliseconds deltaTime)
{
    UpdateDepthFirst(deltaTime, mCurrentCamera);
}

using MatrixStack = std::stack<mat4, std::vector<mat4>>;

static void DrawDepthFirst(
        const mat4& projection,
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

        MatrixStackScope modelViewScope(modelViewStack, node->GetLocalTransform());

        if (node->GetRenderObject() && !node->IsHidden())
        {
            mat4 modelView = modelViewStack.top();

            node->GetRenderObject()->Draw(program, {
                                       { "uProjection", projection },
                                       { "uModelView", modelView }
                                   }, renderState);
        }

        for (const std::shared_ptr<RenderObjectNode>& child : node->GetChildren())
        {
            DrawDepthFirst(projection, modelViewStack, program, renderState, child);
        }
    }
}

void SceneGraph::Draw(
        const std::shared_ptr<IShaderProgram>& program,
        const RenderState& renderState) const
{
    if (!GetCurrentCamera())
    {
        return;
    }

    RenderState decoratedState = renderState;
    decoratedState.Viewport = GetCurrentCamera()->GetViewport();
    decoratedState.ActivatedParameters.set(RenderState::Activate_Viewport);

    if (decoratedState.Viewport == ng::ivec4(0,0,0,1))
    {
        throw std::logic_error("Woops, you forgot to initialize the viewport with meaningful values.");
    }

    // create the viewWorld matrix
    mat4 viewWorld = GetCurrentCamera()->GetLocalTransform();

    for (std::weak_ptr<RenderObjectNode> parent = GetCurrentCamera()->GetParent();
         !parent.expired() && parent.lock() != GetCurrentCamera();
         parent = parent.lock()->GetParent())
    {
        viewWorld = viewWorld * parent.lock()->GetLocalTransform();
    }

    MatrixStack modelViewStack;
    modelViewStack.push(inverse(viewWorld));

    for (const std::shared_ptr<RenderObjectNode>& child : GetCurrentCamera()->GetChildren())
    {
        DrawDepthFirst(GetCurrentCamera()->GetProjection(),
                       modelViewStack, program, decoratedState, child);
    }
}

} // end namespace ng
