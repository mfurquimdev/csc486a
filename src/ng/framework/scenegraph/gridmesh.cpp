#include "ng/framework/scenegraph/gridmesh.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/staticmesh.hpp"

#include "ng/framework/scenegraph/renderobjectnode.hpp"

#include <vector>

namespace ng
{

GridMesh::GridMesh(std::shared_ptr<IRenderer> renderer)
    : mMesh(renderer->CreateStaticMesh())
{ }

void GridMesh::Init(int numColumns, int numRows, vec2 tileSize)
{
    if (numColumns < 1 || numRows < 1)
    {
        throw std::logic_error("Invalid dimensions for GridMesh (must be >= 1)");
    }

    if (tileSize.x < 0 || tileSize.y < 0)
    {
        throw std::logic_error("Invalid tileSize for GridMesh (must be >= 0)");
    }

    std::vector<vec3> gridVertices;

    // add all vertices in the grid
    for (int row = 0; row < numRows + 1; row++)
    {
        for (int col = 0; col < numColumns + 1; col++)
        {
            gridVertices.emplace_back(tileSize.x * col, 0, tileSize.y * row);
            gridVertices.emplace_back(0.0f, 1.0f, 0.0f);
        }
    }

    std::vector<std::uint32_t> gridIndices;

    // add all the indices
    for (int row = 0; row < numRows; row++)
    {
        for (int col = 0; col < numColumns + 1; col++)
        {
            gridIndices.push_back(row * (numColumns + 1) + col);
            gridIndices.push_back((row + 1) * (numColumns + 1) + col);
        }

        // every row except the last one needs degenerate triangles to tie them together
        if (row < numRows - 1)
        {
            gridIndices.push_back((row + 1) * (numColumns + 1) + numColumns);
            gridIndices.push_back((row + 1) * (numColumns + 1));
        }
    }

    VertexFormat gridFormat({
            { VertexAttributeName::Position, VertexAttribute(3, ArithmeticType::Float, false, 2 * sizeof(vec3), 0) },
            { VertexAttributeName::Normal,   VertexAttribute(3, ArithmeticType::Float, false, 2 * sizeof(vec3), sizeof(vec3)) }
        }, ArithmeticType::UInt32);

    std::size_t indexBufferSize = gridIndices.size() * sizeof(gridIndices[0]);
    std::size_t indexCount = gridIndices.size();
    std::size_t vertexCount = gridVertices.size() / 2;

    auto pGridIndices = std::make_shared<std::vector<std::uint32_t>>(std::move(gridIndices));
    std::shared_ptr<const void> indexBuffer(pGridIndices->data(), [pGridIndices](const void*){});

    std::shared_ptr<std::vector<vec3>> pVertexBuffer(new std::vector<vec3>(std::move(gridVertices)));

    std::pair<std::shared_ptr<const void>,std::ptrdiff_t> pBufferData({pVertexBuffer->data(), [pVertexBuffer](const void*){}},
                                                                      vertexCount * 2 * sizeof(vec3));

    mMesh->Init(gridFormat, {
                    { VertexAttributeName::Position, pBufferData },
                    { VertexAttributeName::Normal,   pBufferData }
                }, indexBuffer, indexBufferSize, indexCount);

    mNumColumns = numColumns;
    mNumRows = numRows;
    mTileSize = tileSize;
}

AxisAlignedBoundingBox<float> GridMesh::GetLocalBoundingBox() const
{
    return AxisAlignedBoundingBox<float>(
                vec3(0.0f),
                vec3(mTileSize.x * mNumColumns, 0.0f, mTileSize.y * mNumRows));
}

RenderObjectPass GridMesh::Draw(
        const std::shared_ptr<IShaderProgram>& program,
        const std::map<std::string, UniformValue>& uniforms,
        const RenderState& renderState)
{
    mMesh->Draw(program, uniforms, renderState,
                PrimitiveType::TriangleStrip, 0, mMesh->GetVertexCount());

    return RenderObjectPass::Continue;
}

} // end namespace ng
