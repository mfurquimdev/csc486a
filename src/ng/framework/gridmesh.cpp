#include "ng/framework/gridmesh.hpp"

#include "ng/engine/renderer.hpp"
#include "ng/engine/staticmesh.hpp"

#include <vector>

namespace ng
{

GridMesh::GridMesh(std::shared_ptr<IRenderer> renderer)
    : mMesh(renderer->CreateStaticMesh())
{ }

void GridMesh::Init(int numCols, int numRows, vec2 tileSize)
{
    if (numCols < 1 || numRows < 1)
    {
        throw std::logic_error("Invalid dimensions for GridMesh (must be >= 1)");
    }

    if (tileSize.x < 0 || tileSize.y < 0)
    {
        throw std::logic_error("Invalid tileSize for GridMesh (must be >= 0)");
    }

    std::vector<ng::vec3> gridVertices;

    // add all vertices in the grid
    for (int row = 0; row < numRows + 1; row++)
    {
        for (int col = 0; col < numCols + 1; col++)
        {
            gridVertices.emplace_back(tileSize.x * col, 0, tileSize.y * row);
        }
    }

    std::vector<std::uint32_t> gridIndices;

    // add all the indices
    for (int row = 0; row < numRows; row++)
    {
        for (int col = 0; col < numCols + 1; col++)
        {
            gridIndices.push_back(row * (numCols + 1) + col);
            gridIndices.push_back((row + 1) * (numCols + 1) + col);
        }

        // every row except the last one needs degenerate triangles to tie them together
        if (row < numRows - 1)
        {
            gridIndices.push_back((row + 1) * (numCols + 1) + numCols);
            gridIndices.push_back((row + 1) * (numCols + 1));
        }
    }

    std::size_t indexBufferSize = gridIndices.size() * sizeof(gridIndices[0]);
    std::size_t indexCount = gridIndices.size();

    auto pGridIndices = std::make_shared<std::vector<std::uint32_t>>(std::move(gridIndices));
    std::shared_ptr<const void> indexBuffer(pGridIndices->data(), [pGridIndices](const void*){});

    auto pGridPositions = std::make_shared<std::vector<vec3>>(std::move(gridVertices));
    std::shared_ptr<const void> vertexBuffer(pGridPositions->data(), [pGridPositions](const void*){});

    VertexFormat gridFormat({
            { VertexAttributeName::Position, VertexAttribute(3, ArithmeticType::Float, false, 0, 0) }
        }, ArithmeticType::UInt32);

    mMesh->Init(gridFormat, {
                    { VertexAttributeName::Position, { vertexBuffer, pGridPositions->size() * sizeof(vec3) } }
                }, indexBuffer, indexBufferSize, indexCount);
}

void GridMesh::Draw(const std::shared_ptr<IShaderProgram>& program,
          const std::map<std::string, UniformValue>& uniforms,
          const RenderState& renderState)
{
    mMesh->Draw(program, uniforms, renderState,
                PrimitiveType::TriangleStrip, 0, mMesh->GetVertexCount());
}

} // end namespace ng
