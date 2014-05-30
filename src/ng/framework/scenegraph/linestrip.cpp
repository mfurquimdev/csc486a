#include "ng/framework/scenegraph/linestrip.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/staticmesh.hpp"

#include "ng/framework/scenegraph/renderobjectnode.hpp"

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

void LineStrip::RemovePoint(std::vector<vec3>::const_iterator which)
{
    mPoints.erase(mPoints.begin() + std::distance(mPoints.cbegin(), which));

    // rebuild bounding box
    mBoundingBox = ng::AxisAlignedBoundingBox<float>();
    for (vec3 point : mPoints)
    {
        mBoundingBox.AddPoint(point);
    }

    mIsMeshDirty = true;
}

void LineStrip::SetPoint(std::vector<vec3>::const_iterator which, vec3 value)
{
    mPoints.at(which - mPoints.begin()) = value;
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
