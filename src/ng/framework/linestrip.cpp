#include "ng/framework/linestrip.hpp"

#include "ng/engine/renderer.hpp"
#include "ng/engine/staticmesh.hpp"

#include "ng/framework/renderobjectnode.hpp"

namespace ng
{

LineStrip::LineStrip(std::shared_ptr<IRenderer> renderer)
    : mMesh(renderer->CreateStaticMesh())
{ }

void LineStrip::Reset()
{
    mPoints.clear();
    mBoundingBox = AxisAlignedBoundingBox<float>();
    mIsMeshDirty = true;
}

void LineStrip::AddPoint(vec3 point)
{
    mPoints.push_back(point);
    mBoundingBox.AddPoint(point);
    mIsMeshDirty = true;
}

RenderObjectPass LineStrip::PreUpdate(std::chrono::milliseconds,
                                      RenderObjectNode& node)
{
    node.SetLocalBoundingBox(mBoundingBox);

    return RenderObjectPass::Continue;
}

RenderObjectPass LineStrip::Draw(
        const std::shared_ptr<IShaderProgram>& program,
        const std::map<std::string, UniformValue>& uniforms,
        const RenderState& renderState)
{
    if (mIsMeshDirty)
    {
        VertexFormat vertexFormat({
                { VertexAttributeName::Position, VertexAttribute(3, ArithmeticType::Float, false, 0, 0) }
            });

        std::unique_ptr<vec3[]> vertexBuffer(new vec3[mPoints.size()]);
        std::copy(mPoints.begin(), mPoints.end(), &vertexBuffer[0]);

        mMesh->Init(vertexFormat, {
                        { VertexAttributeName::Position, { std::move(vertexBuffer), mPoints.size() * sizeof(vec3) } }
                    }, nullptr, 0, mPoints.size());

        mIsMeshDirty = false;
    }

    auto extraUniforms = uniforms;
    extraUniforms.emplace("uTint", vec4(1,1,1,1));

    mMesh->Draw(program, extraUniforms, renderState,
                PrimitiveType::LineStrip, 0, mMesh->GetVertexCount());

    return RenderObjectPass::Continue;}

} // end namespace ng
