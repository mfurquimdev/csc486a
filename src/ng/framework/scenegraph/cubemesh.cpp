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
    //        e----------f
    //       /|         /|
    //      g----------h |
    //      | |        | |
    //      | |        | |
    //      | a--------|-b
    //      |/         |/
    //      c----------d
    //
    // minExtent = vertex 0
    // maxExtent = vertex 7
    //
    //              -z
    //               ^
    //         +y   /
    //          ^  /
    //          | /
    //    -x <-- --> +x
    //         /|
    //        / v
    //       / -y
    //      v
    //     +z

    // positions
    vec3 minExtent = vec3(-sideLength / 2);
    vec3 maxExtent = vec3( sideLength / 2);

    vec3 a = minExtent;
    vec3 b = minExtent + vec3(sideLength,0,0);
    vec3 c = minExtent + vec3(0,0,sideLength);
    vec3 d = minExtent + vec3(sideLength,0,sideLength);
    vec3 e = maxExtent - vec3(sideLength,0,sideLength);
    vec3 f = maxExtent - vec3(0,0,sideLength);
    vec3 g = maxExtent - vec3(sideLength,0,0);
    vec3 h = maxExtent;

    // normals
    vec3 top(0,1,0);
    vec3 bottom(0,-1,0);
    vec3 left(-1,0,0);
    vec3 right(1,0,0);
    vec3 front(0,0,1);
    vec3 back(0,0,-1);

    std::shared_ptr<std::vector<vec3>> pVertexBuffer(new std::vector<vec3> {
        a,bottom, b,bottom, c,bottom,   b,bottom, d,bottom, c,bottom,   // bottom face
        c,front,  d,front,  g,front,    d,front,  h,front,  g,front,    // front face
        a,left,   c,left,   e,left,     c,left,   g,left,   e,left,     // left face
        b,back,   a,back,   f,back,     a,back,   e,back,   f,back,     // back face
        d,right,  b,right,  h,right,    b,right,  f,right,  h,right,    // right face
        g,top,    h,top,    e,top,      h,top,    f,top,    e,top       // top face
    });

    std::size_t numVertices = pVertexBuffer->size() / 2;

    VertexFormat cubeFormat({
            { VertexAttributeName::Position, VertexAttribute(3, ArithmeticType::Float, false, 2 * sizeof(vec3), 0) },
            { VertexAttributeName::Normal,   VertexAttribute(3, ArithmeticType::Float, false, 2 * sizeof(vec3), sizeof(vec3)) }
        });

    std::pair<std::shared_ptr<const void>,std::ptrdiff_t> pBufferData({pVertexBuffer->data(), [pVertexBuffer](const void*){}},
                                                                      numVertices * 2 * sizeof(vec3));

    mMesh->Init(cubeFormat, {
                    { VertexAttributeName::Position, pBufferData },
                    { VertexAttributeName::Normal,   pBufferData }
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
                PrimitiveType::Triangles, 0, mMesh->GetVertexCount());

    return RenderObjectPass::Continue;
}

} // end namespace ng
