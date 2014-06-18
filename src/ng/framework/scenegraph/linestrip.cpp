#include "ng/framework/scenegraph/linestrip.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/staticmesh.hpp"

#include "ng/framework/scenegraph/renderobjectnode.hpp"

namespace ng
{

LineStrip::LineStrip(std::shared_ptr<IRenderer> renderer)
    : mMesh(renderer->CreateStaticMesh())
{ }

void LineStrip::UpdatePoints(const vec3* points, std::size_t numPoints)
{
    mPoints = std::vector<vec3>(points, points + numPoints);
    mIsMeshDirty = true;
}

void LineStrip::UpdatePoints(std::vector<vec3> points)
{
    mPoints = std::move(points);
    mIsMeshDirty = true;
}

AxisAlignedBoundingBox<float> LineStrip::GetLocalBoundingBox() const
{
    return mBoundingBox;
}

RenderObjectPass LineStrip::Draw(
        const std::shared_ptr<IShaderProgram>& program,
        const std::map<std::string, UniformValue>& uniforms,
        const RenderState& renderState)
{
    if (mPoints.size() != 0)
    {
        if (mIsMeshDirty)
        {
            VertexFormat vertexFormat({
                    { VertexAttributeName::Position, VertexAttribute(3, ArithmeticType::Float, false, 0, 0) }
                });

            std::unique_ptr<vec3[]> vertexBuffer(new vec3[mPoints.size()]);
            std::shared_ptr<std::vector<vec3>> pVertexBuffer(new std::vector<vec3>(mPoints.size()));
            std::copy(mPoints.begin(), mPoints.end(), pVertexBuffer->data());

            mMesh->Init(vertexFormat, {
                            { VertexAttributeName::Position, { std::shared_ptr<const void>(pVertexBuffer->data(), [pVertexBuffer](const void*){}),
                                                               mPoints.size() * sizeof(vec3) } }
                        }, nullptr, 0, mPoints.size());

            mIsMeshDirty = false;
        }

        mMesh->Draw(program, uniforms, renderState,
                    PrimitiveType::LineStrip, 0, mMesh->GetVertexCount());
    }

    return RenderObjectPass::Continue;
}

} // end namespace ng
