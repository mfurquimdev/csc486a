#include "ng/framework/scenegraph/scenegraph.hpp"

#include "ng/engine/rendering/uniform.hpp"

#include "ng/framework/scenegraph/camera.hpp"
#include "ng/framework/scenegraph/light.hpp"

#include "ng/framework/scenegraph/renderobject.hpp"
#include "ng/framework/scenegraph/renderobjectnode.hpp"

#include "ng/engine/rendering/renderstate.hpp"
#include "ng/engine/util/scopeguard.hpp"

#include <stack>
#include <algorithm>

namespace ng
{

void SceneGraph::SetUpdateRoot(std::shared_ptr<RenderObjectNode> updateRoot)
{
    mUpdateRoot = std::move(updateRoot);
}

void SceneGraph::AddCamera(std::shared_ptr<CameraNode> currentCamera)
{
    mCameras.push_back(std::move(currentCamera));
}

void SceneGraph::AddLight(std::shared_ptr<LightNode> light)
{
    mLights.push_back(std::move(light));
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
    UpdateDepthFirst(deltaTime, mUpdateRoot);
}

using MatrixStack = std::stack<mat4, std::vector<mat4>>;

static void DrawMultiPassDepthFirst(
        const mat4& projection,
        MatrixStack& modelViewStack,
        const std::shared_ptr<IShaderProgram>& program,
        const RenderState& renderState,
        const std::vector<std::shared_ptr<LightNode>>& lights,
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

        RenderObjectPass pass = RenderObjectPass::Continue;

        if (node->GetRenderObject() && !node->IsHidden())
        {
            mat4 modelView = modelViewStack.top();

            std::map<std::string,UniformValue> uniforms{
                { "uProjection", projection },
                { "uModelView", modelView },
                { "uNormalMatrix", mat3(inverse(transpose(modelView))) }
            };

            for (const std::shared_ptr<LightNode>& light : lights)
            {
                // TODO: fix this
                vec3 lightViewPos = light->GetWorldBoundingBox().GetCenter();
                uniforms["uLight.Position"] = UniformValue(vec3(lightViewPos));
                uniforms["uLight.Color"] = UniformValue(light->GetLight()->GetColor());

                pass = node->GetRenderObject()->Draw(program, uniforms, renderState);
            }
        }

        if (pass != RenderObjectPass::SkipChildren)
        {
            for (const std::shared_ptr<RenderObjectNode>& child : node->GetChildren())
            {
                DrawMultiPassDepthFirst(projection, modelViewStack,
                                        program, renderState, lights, child);
            }
        }
    }
}

void SceneGraph::DrawMultiPass(
        const std::shared_ptr<IShaderProgram>& program,
        const RenderState& renderState) const
{
    for (const std::shared_ptr<CameraNode>& camera : mCameras)
    {
        camera->GetCamera()->mIsCurrentCamera = true;
        auto currentCameraGuard = make_scope_guard([&]{
            camera->GetCamera()->mIsCurrentCamera = false;
        });

        RenderState decoratedState = renderState;
        decoratedState.Viewport = camera->GetViewport();
        decoratedState.ActivatedParameters.set(RenderState::Activate_Viewport);

        if (decoratedState.Viewport == ng::ivec4(0,0,0,1))
        {
            throw std::logic_error("Woops, you probably forgot to initialize the viewport with meaningful values.");
        }

        // create the viewWorld matrix
        mat4 viewWorld = camera->GetLocalTransform();

        // add in all transformations of parents of the camera
        for (std::weak_ptr<RenderObjectNode> parent = camera->GetParent();
             !parent.expired() && parent.lock() != camera;
             parent = parent.lock()->GetParent())
        {
            viewWorld = viewWorld * parent.lock()->GetLocalTransform();
        }

        MatrixStack modelViewStack;
        modelViewStack.push(inverse(viewWorld));

        for (const std::shared_ptr<RenderObjectNode>& child : camera->GetChildren())
        {
            DrawMultiPassDepthFirst(camera->GetProjection(),
                           modelViewStack, program, decoratedState, mLights, child);
        }
    }
}

} // end namespace ng
