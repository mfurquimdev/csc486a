#include "ng/engine/rendering/loopsubdivisionmesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"
#include "ng/engine/util/debug.hpp"

#include <cmath>

namespace ng
{

LoopSubdivisionMesh::LoopSubdivisionMesh(
        std::shared_ptr<IMesh> meshToSubdivide)
    :
      mMeshToSubdivide(std::move(meshToSubdivide))
{
    if (mMeshToSubdivide == nullptr)
    {
        throw std::logic_error("Cannot subdivide null mesh");
    }
}

static std::vector<std::reference_wrapper<VertexAttribute>>
    GetEnabledAttributes(VertexFormat& fmt)
{
    std::vector<std::reference_wrapper<VertexAttribute>> attribs;

    if (fmt.Position.Enabled)
    {
        attribs.push_back(fmt.Position);
    }
    if (fmt.Normal.Enabled)
    {
        attribs.push_back(fmt.Normal);
    }
    if (fmt.TexCoord.Enabled)
    {
        attribs.push_back(fmt.TexCoord);
    }

    return attribs;
}

VertexFormat LoopSubdivisionMesh::GetVertexFormat() const
{
    VertexFormat fmt;

    VertexFormat baseFmt = mMeshToSubdivide->GetVertexFormat();
    fmt = baseFmt;

    fmt.IsIndexed = false;

    auto enabledAttribs = GetEnabledAttributes(fmt);

    std::size_t vertexSize = 0;
    for (const VertexAttribute& attrib : enabledAttribs)
    {
        vertexSize += attrib.Cardinality * SizeOfArithmeticType(attrib.Type);
    }

    std::size_t currentOffset = 0;
    for (VertexAttribute& attrib : enabledAttribs)
    {
        attrib.Offset = currentOffset;
        attrib.Stride = vertexSize;

        currentOffset += attrib.Cardinality * SizeOfArithmeticType(attrib.Type);
    }

    return fmt;
}

std::size_t LoopSubdivisionMesh::GetMaxVertexBufferSize() const
{
    std::size_t baseNumVertices = mMeshToSubdivide->WriteIndices(nullptr);
    if (baseNumVertices == 0)
    {
        baseNumVertices = mMeshToSubdivide->WriteVertices(nullptr);
    }

    baseNumVertices *= 4;

    VertexFormat fmt = GetVertexFormat();

    std::size_t vertexSize = 0;
    for (const VertexAttribute& attrib : GetEnabledAttributes(fmt))
    {
        vertexSize += attrib.Cardinality * SizeOfArithmeticType(attrib.Type);
    }

    return vertexSize * baseNumVertices;
}

std::size_t LoopSubdivisionMesh::GetMaxIndexBufferSize() const
{
    return 0;
}

std::size_t LoopSubdivisionMesh::WriteVertices(void* buffer) const
{
    VertexFormat fmt = GetVertexFormat();

    std::unique_ptr<char[]> baseVertices;
    std::size_t numBaseVertices = 0;
    if (mMeshToSubdivide->GetMaxVertexBufferSize() > 0)
    {
        baseVertices.reset(new char[mMeshToSubdivide->GetMaxVertexBufferSize()]);
        numBaseVertices = mMeshToSubdivide->WriteVertices(baseVertices.get());
    }

    std::unique_ptr<char[]> baseIndices;
    std::size_t numBaseIndices = 0;
    if (mMeshToSubdivide->GetMaxIndexBufferSize() > 0)
    {
        baseIndices.reset(new char[mMeshToSubdivide->GetMaxIndexBufferSize()]);
        numBaseIndices = mMeshToSubdivide->WriteIndices(baseIndices.get());
    }

    std::size_t numFaces = (numBaseIndices > 0 ?
                                numBaseIndices : numBaseVertices) / 3;

    using sizevec3 = vec<std::size_t,3>;

    auto getFaceVertices = [&](std::size_t face) -> sizevec3
    {
        if (numBaseIndices > 0)
        {
            void* indexOffset = baseIndices.get() + fmt.IndexOffset;

            if (fmt.IndexType == ArithmeticType::UInt8)
            {
                const std::uint8_t* idx =
                        static_cast<const std::uint8_t*>(indexOffset);

                return sizevec3(
                            idx[3 * face],
                            idx[3 * face + 1],
                            idx[3 * face + 2]);
            }
            else if (fmt.IndexType == ArithmeticType::UInt16)
            {
                const std::uint16_t* idx =
                        static_cast<const std::uint16_t*>(indexOffset);

                return sizevec3(
                            idx[3 * face],
                            idx[3 * face + 1],
                            idx[3 * face + 2]);
            }
            else if (fmt.IndexType == ArithmeticType::UInt32)
            {
                const std::uint32_t* idx =
                        static_cast<const std::uint32_t*>(indexOffset);

                return sizevec3(
                            idx[3 * face],
                            idx[3 * face + 1],
                            idx[3 * face + 2]);
            }
            else
            {
                throw std::logic_error("Unhandled IndexType");
            }
        }
        else
        {
            return sizevec3(
                        3 * face,
                        3 * face + 1,
                        3 * face + 2);
        }
    };

    DebugPrintf("LoopSubdivision face list:\n");
    for (std::size_t faceIdx = 0; faceIdx < numFaces; faceIdx++)
    {
        sizevec3 face = getFaceVertices(faceIdx);
        DebugPrintf("got face: (%u, %u, %u)\n", face[0], face[1], face[2]);
    }

    return (numBaseIndices > 0 ?
            numBaseIndices : numBaseVertices) * 4;
}

std::size_t LoopSubdivisionMesh::WriteIndices(void*) const
{
    return 0;
}


} // end namespace ng
