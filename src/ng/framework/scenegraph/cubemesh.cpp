#include "ng/framework/scenegraph/cubemesh.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/staticmesh.hpp"

#include "ng/framework/scenegraph/renderobjectnode.hpp"

#include <vector>

namespace ng
{

CubeMesh::CubeMesh(std::shared_ptr<IRenderer> renderer)
    : mMesh(renderer->CreateStaticMesh())
{ }

void CubeMesh::Init(float sideLength)
{
    VertexFormat cubeFormat({
            { VertexAttributeName::Position, VertexAttribute(3, ArithmeticType::Float, false, 0, 0) }
        });

    // http://codedot.livejournal.com/109158.html
    constexpr std::size_t numVertices = 14;
    std::unique_ptr<vec3[]> vertexBuffer(new vec3[numVertices] {
        {   sideLength / 2, - sideLength / 2, - sideLength / 2 }, // A
        {   sideLength / 2, - sideLength / 2,   sideLength / 2 }, // B
        { - sideLength / 2, - sideLength / 2, - sideLength / 2 }, // C
        { - sideLength / 2, - sideLength / 2,   sideLength / 2 }, // D
        { - sideLength / 2,   sideLength / 2,   sideLength / 2 }, // E
        {   sideLength / 2, - sideLength / 2,   sideLength / 2 }, // B
        {   sideLength / 2,   sideLength / 2,   sideLength / 2 }, // F
        {   sideLength / 2,   sideLength / 2, - sideLength / 2 }, // G
        { - sideLength / 2,   sideLength / 2,   sideLength / 2 }, // E
        { - sideLength / 2,   sideLength / 2, - sideLength / 2 }, // H
        { - sideLength / 2, - sideLength / 2, - sideLength / 2 }, // C
        {   sideLength / 2,   sideLength / 2, - sideLength / 2 }, // G
        {   sideLength / 2, - sideLength / 2, - sideLength / 2 }, // A
        {   sideLength / 2, - sideLength / 2,   sideLength / 2 }, // B
    });

    mMesh->Init(cubeFormat, {
                    { VertexAttributeName::Position, { std::move(vertexBuffer), numVertices * sizeof(vec3) } }
                }, nullptr, 0, numVertices);

    mSideLength = sideLength;
}

AxisAlignedBoundingBox<float> CubeMesh::GetLocalBoundingBox() const
{
    return AxisAlignedBoundingBox<float>(
                vec3(-mSideLength / 2),
                vec3(mSideLength / 2));
}

RenderObjectPass CubeMesh::Draw(
        const std::shared_ptr<IShaderProgram>& program,
        const std::map<std::string, UniformValue>& uniforms,
        const RenderState& renderState)
{
    auto extraUniforms = uniforms;
    extraUniforms.emplace("uTint", vec4(1,1,0,1));

    mMesh->Draw(program, extraUniforms, renderState,
                PrimitiveType::TriangleStrip, 0, mMesh->GetVertexCount());

    return RenderObjectPass::Continue;
}

} // end namespace ng
