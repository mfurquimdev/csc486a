#include "ng/framework/renderobjectmanager.hpp"

#include "ng/engine/uniform.hpp"

#include "ng/framework/renderobject.hpp"

#include <stack>

namespace ng
{

void RenderObjectManager::Update(std::chrono::milliseconds deltaTime)
{

}

using MatrixStack = std::stack<mat4, std::vector<mat4>>;

static void DrawDepthFirst(
        MatrixStack& projectionStack,
        MatrixStack& modelViewStack,
        const std::shared_ptr<IShaderProgram>& program,
        const RenderState& renderState,
        const std::shared_ptr<RenderObjectManager::ObjectNode>& node)
{
    if (node)
    {
        struct MatrixStackScope
        {
            MatrixStack& mMatStack;

            MatrixStackScope(MatrixStack& matStack, mat4 m)
                : mMatStack(matStack)
            {
                mMatStack.push(mMatStack.top() * m);
            }

            ~MatrixStackScope()
            {
                mMatStack.pop();
            }
        };

        MatrixStackScope projectionScope(modelViewStack, node->ProjectionTransform);
        MatrixStackScope modelViewScope(modelViewStack, node->ModelViewTransform);

        node->Object->Draw(program, {
                               { "uProjection" , projectionStack.top() },
                               { "uModelView" , modelViewStack.top() }
                           }, renderState);

        for (const std::shared_ptr<RenderObjectManager::ObjectNode>& child : node->ChildNodes)
        {
            DrawDepthFirst(projectionStack, modelViewStack, program, renderState, child);
        }
    }
}

void RenderObjectManager::Draw(
        const std::shared_ptr<IShaderProgram>& program,
        const RenderState& renderState) const
{
    MatrixStack modelViewStack, projectionStack;
    modelViewStack.push(mat4());
    projectionStack.push(mat4());

    for (const std::shared_ptr<ObjectNode>& root : GetRoots())
    {
        DrawDepthFirst(projectionStack, modelViewStack, program, renderState, root);
    }
}

} // end namespace ng
