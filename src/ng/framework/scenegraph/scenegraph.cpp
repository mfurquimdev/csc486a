#include "ng/framework/scenegraph/scenegraph.hpp"

#include "ng/engine/rendering/uniform.hpp"

#include "ng/framework/scenegraph/camera.hpp"
#include "ng/framework/scenegraph/light.hpp"

#include "ng/framework/scenegraph/renderobject.hpp"
#include "ng/framework/scenegraph/renderobjectnode.hpp"

#include "ng/engine/rendering/renderstate.hpp"
#include "ng/engine/util/scopeguard.hpp"

#include "ng/framework/scenegraph/shaderprofile.hpp"

#include <stack>
#include <algorithm>

namespace ng
{

void SceneGraph::SetRoot(std::shared_ptr<RenderObjectNode> root)
{
    mRoot = std::move(root);
}

void SceneGraph::SetCamera(std::shared_ptr<CameraNode> camera)
{
    mCamera = std::move(camera);
}

void SceneGraph::AddLight(std::weak_ptr<LightNode> light)
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
    UpdateDepthFirst(deltaTime, mRoot);
}

using MatrixStack = std::stack<mat4, std::vector<mat4>>;

static void DrawMultiPassDepthFirst(
        const mat4& projection,
        const mat4& worldView,
        MatrixStack& modelViewStack,
        const ShaderProfile& profile,
        const RenderState& renderState,
        const std::vector<std::weak_ptr<LightNode>>& lights,
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

            const Material& mat = node->GetMaterial();
            std::shared_ptr<IShaderProgram> program = profile.GetProgramForMaterial(mat);

            std::map<std::string,UniformValue> uniforms{
                { "uProjection", projection },
                { "uModelView", modelView },
                { "uTint", mat.Tint }
            };

            if (mat.Style == MaterialStyle::Debug)
            {
                // ignore lighting. just draw.
                pass = node->GetRenderObject()->Draw(program, uniforms, renderState);
            }
            else
            {
                bool drewOnce = false;

                for (const std::weak_ptr<LightNode>& wpLight : lights)
                {
                    if (wpLight.expired())
                    {
                        continue;
                    }

                    std::shared_ptr<LightNode> light = wpLight.lock();

                    // check if the light intersects with the node
                    if (!AABBoxIntersect(light->GetWorldBoundingBox(), node->GetWorldBoundingBox()))
                    {
                        continue;
                    }

                    vec4 lightViewPos = worldView * light->GetWorldTransform() * vec4(0,0,0,1);
                    vec4 lightModelPos = inverse(modelView) * lightViewPos;

                    vec4 lightExtentViewPos = worldView * light->GetWorldTransform() * vec4(light->GetLight()->GetRadius(),0,0,1);
                    vec4 lightExtentModelPos = inverse(modelView) * lightExtentViewPos;

                    uniforms["uLight.Position"] = UniformValue(vec3(lightModelPos));
                    uniforms["uLight.Radius"] = UniformValue(length(vec3(lightExtentModelPos - lightModelPos)));
                    uniforms["uLight.Color"] = UniformValue(light->GetLight()->GetColor());

                    RenderState decoratedState = renderState;

                    if (drewOnce)
                    {
                        decoratedState.DepthTestFunc = DepthTestFunc::Equal;

                        decoratedState.BlendingEnabled = true;

                        decoratedState.SourceBlendMode = BlendMode::One;

                        decoratedState.DestinationBlendMode = BlendMode::One;
                    }

                    pass = node->GetRenderObject()->Draw(program, uniforms, decoratedState);
                    drewOnce = true;
                }
                }
        }

        if (pass != RenderObjectPass::SkipChildren)
        {
            for (const std::shared_ptr<RenderObjectNode>& child : node->GetChildren())
            {
                DrawMultiPassDepthFirst(projection, worldView, modelViewStack,
                                        profile, renderState, lights, child);
            }
        }
    }
}

void SceneGraph::DrawMultiPass(const ShaderProfile& profile) const
{
    const std::shared_ptr<CameraNode>& camera = mCamera;

    RenderState renderState;

    renderState.DepthTestEnabled = true;

    renderState.Viewport = camera->GetViewport();

    if (renderState.Viewport == ng::ivec4(0,0,0,1))
    {
        throw std::logic_error("Woops, you probably forgot to initialize the viewport with meaningful values.");
    }

    mat4 worldView = camera->GetWorldView();

    MatrixStack modelViewStack;
    modelViewStack.push(worldView);

    DrawMultiPassDepthFirst(camera->GetProjection(), worldView, modelViewStack,
                            profile, renderState, mLights, mRoot);
}

} // end namespace ng
