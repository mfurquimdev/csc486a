#include "ng/framework/scenegraph/uvsphere.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/staticmesh.hpp"
#include "ng/engine/math/constants.hpp"

#include "ng/framework/scenegraph/renderobjectnode.hpp"

#include "ng/engine/util/debug.hpp"

#include <vector>

namespace ng
{

UVSphere::UVSphere(std::shared_ptr<IRenderer> renderer)
    : mMesh(renderer->CreateStaticMesh())
{ }

void UVSphere::Init(int numRings, int numSegments, float radius)
{
    if (numRings < 3 || numSegments < 3 || radius < 0)
    {
        throw std::logic_error("Invalid dimensions for UVSphere.");
    }

    std::vector<vec3> sphereVertices;

    float radiansPerRing = 2 * pi<float>::value / numRings;
    float radiansPerSegment = pi<float>::value / numSegments;

    // add all vertices in the sphere
    for (int segment = 0; segment < numSegments + 1; segment++)
    {
        float theta = radiansPerSegment * segment;

        for (int ring = 0; ring <= numRings; ring++)
        {
            float phi = radiansPerRing * ring;

            vec3 normal(std::sin(theta) * std::cos(phi),
                        std::cos(theta),
                        std::sin(theta) * std::sin(phi));

            sphereVertices.push_back(radius * normal);
            sphereVertices.push_back(normal);
        }
    }

    std::vector<std::uint32_t> sphereIndices;

    // add all the indices
    for (int segment = 0; segment < numSegments; segment++)
    {
        for (int ring = 0; ring < numRings + 1; ring++)
        {
            sphereIndices.push_back(segment * (numRings + 1) + ring);
            sphereIndices.push_back((segment + 1) * (numRings + 1) + ring);
        }

        // every segment except the last one needs degenerate triangles to tie them together
        if (segment < numSegments - 1)
        {
            sphereIndices.push_back((segment + 2) * numRings - 1);
        }
    }

    VertexFormat gridFormat({
            { VertexAttributeName::Position, VertexAttribute(3, ArithmeticType::Float, false, sizeof(vec3) * 2, 0) },
            { VertexAttributeName::Normal,   VertexAttribute(3, ArithmeticType::Float, false, sizeof(vec3) * 2, sizeof(vec3)) }
        }, ArithmeticType::UInt32);

    std::size_t indexBufferSize = sphereIndices.size() * sizeof(std::uint32_t);
    std::size_t indexCount = sphereIndices.size();

    auto pSphereIndices = std::make_shared<std::vector<std::uint32_t>>(std::move(sphereIndices));
    std::shared_ptr<const void> indexBuffer(pSphereIndices->data(), [pSphereIndices](const void*){});

    auto pSphereVertices = std::make_shared<std::vector<vec3>>(std::move(sphereVertices));
    std::pair<std::shared_ptr<const void>,std::ptrdiff_t> pBufferData({pSphereVertices->data(), [pSphereVertices](const void*){}},
                                                                       pSphereVertices->size() * sizeof(vec3));

    mMesh->Init(gridFormat, {
                    { VertexAttributeName::Position, pBufferData },
                    { VertexAttributeName::Normal,   pBufferData }
                }, indexBuffer, indexBufferSize, indexCount);

    mNumRings = numRings;
    mNumSegments = numSegments;
    mRadius = radius;
}

AxisAlignedBoundingBox<float> UVSphere::GetLocalBoundingBox() const
{
    return AxisAlignedBoundingBox<float>(-vec3(mRadius), vec3(mRadius));
}

RenderObjectPass UVSphere::Draw(
        const std::shared_ptr<IShaderProgram>& program,
        const std::map<std::string, UniformValue>& uniforms,
        const RenderState& renderState)
{
    mMesh->Draw(program, uniforms, renderState,
                PrimitiveType::TriangleStrip, 0, mMesh->GetVertexCount());

    return RenderObjectPass::Continue;
}

} // end namespace ng
