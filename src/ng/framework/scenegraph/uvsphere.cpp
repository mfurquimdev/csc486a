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

            sphereVertices.push_back(radius * vec3(std::sin(theta) * std::cos(phi),
                                                   std::cos(theta),
                                                   std::sin(theta) * std::sin(phi)));
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

    std::size_t indexBufferSize = sphereIndices.size() * sizeof(std::uint32_t);
    std::size_t indexCount = sphereIndices.size();

    DebugPrintf("Creating UVSphere with %d elements\n", indexCount);
    DebugPrintf("Vertices:\n");
    for (size_t i = 0; i < sphereVertices.size(); i++) {
        auto v = sphereVertices[i];
        DebugPrintf("%d: {%f,%f,%f}\n", i, v.x, v.y, v.z);
    }
    DebugPrintf("\nIndices:\n");
    for (auto i : sphereIndices) DebugPrintf("%d ", i);
    DebugPrintf("\n");

    auto pSphereIndices = std::make_shared<std::vector<std::uint32_t>>(std::move(sphereIndices));
    std::shared_ptr<const void> indexBuffer(pSphereIndices->data(), [pSphereIndices](const void*){});

    auto pSphereVertices = std::make_shared<std::vector<vec3>>(std::move(sphereVertices));
    std::shared_ptr<const void> vertexBuffer(pSphereVertices->data(), [pSphereVertices](const void*){});

    VertexFormat gridFormat({
            { VertexAttributeName::Position, VertexAttribute(3, ArithmeticType::Float, false, 0, 0) }
        }, ArithmeticType::UInt32);


    mMesh->Init(gridFormat, {
                    { VertexAttributeName::Position, { vertexBuffer, pSphereVertices->size() * sizeof(vec3) } }
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
    auto extraUniforms = uniforms;
    extraUniforms.emplace("uTint", vec4(1,0,0,1));

    mMesh->Draw(program, extraUniforms, renderState,
                PrimitiveType::TriangleStrip, 0, mMesh->GetVertexCount());

    return RenderObjectPass::Continue;
}

} // end namespace ng
