#include "ng/framework/meshes/loopsubdivisionmesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"
#include "ng/engine/util/debug.hpp"

#include <array>
#include <cmath>
#include <cstring>

namespace ng
{

LoopSubdivisionMesh::LoopSubdivisionMesh(
        std::shared_ptr<IMesh> meshToSubdivide,
        int numSubdivisions)
    :
      mMeshToSubdivide(std::move(meshToSubdivide)),
      mNumSubdivisons(numSubdivisions)
{
    if (mMeshToSubdivide == nullptr)
    {
        throw std::logic_error("Cannot subdivide null mesh");
    }

    if (mNumSubdivisons < 0)
    {
        throw std::logic_error("number of subdivisions must be >= 0");
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

    baseNumVertices *= std::pow(4, mNumSubdivisons);

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
    VertexFormat baseFmt = mMeshToSubdivide->GetVertexFormat();

    std::unique_ptr<char[]> baseVertices;
    std::size_t numBaseVertices = 0;
    if (mMeshToSubdivide->GetMaxVertexBufferSize() > 0)
    {
        if (buffer)
        {
            baseVertices.reset(
                new char[mMeshToSubdivide->GetMaxVertexBufferSize()]);
        }

        numBaseVertices = mMeshToSubdivide->WriteVertices(baseVertices.get());
    }

    std::unique_ptr<char[]> baseIndices;
    std::size_t numBaseIndices = 0;
    if (mMeshToSubdivide->GetMaxIndexBufferSize() > 0)
    {
        if (buffer)
        {
            baseIndices.reset(
                        new char[mMeshToSubdivide->GetMaxIndexBufferSize()]);
        }

        numBaseIndices = mMeshToSubdivide->WriteIndices(baseIndices.get());
    }

    if (buffer != nullptr)
    {
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

        std::size_t vertexSize = 0;
        for (const VertexAttribute& attrib : GetEnabledAttributes(fmt))
        {
            vertexSize += attrib.Cardinality * SizeOfArithmeticType(attrib.Type);
        }

        //          v2           //
        //          / \          //
        //         /   \         //
        //        / tr4 \        //
        //       /       \       //
        //     e2 ------ e1      //
        //     / \       / \     //
        //    /   \ tr2 /   \    //
        //   / tr1 \   / tr3 \   //
        //  /       \ /       \  //
        // v0 ----- e0 ------ v1 //

        // storage for v0, v1, v2
        std::array<std::unique_ptr<char[]>,3> vertexData;
        for (std::size_t i = 0; i < vertexData.size(); i++)
        {
            vertexData[i].reset(new char[vertexSize]);
        }

        // storage for e0, e1, e2
        std::array<std::unique_ptr<char[]>,3> edgeData;
        for (std::size_t i = 0; i < edgeData.size(); i++)
        {
            edgeData[i].reset(new char[vertexSize]);
        }

        std::size_t bufferOffset = 0;

        for (std::size_t faceIdx = 0; faceIdx < numFaces; faceIdx++)
        {
            // corresponds to indices for v0, v1, v2
            sizevec3 face = getFaceVertices(faceIdx);

            // byte offset into the current vertex
            std::array<std::size_t,3> offsets = {0, 0, 0};

            for (const VertexAttribute& attrib : GetEnabledAttributes(baseFmt))
            {
                std::size_t attribSize =
                    attrib.Cardinality * SizeOfArithmeticType(attrib.Type);

                // get the data for each vertex
                for (std::size_t v = 0; v < vertexData.size(); v++)
                {
                    std::memcpy(
                        vertexData[v].get() + offsets[v],
                        baseVertices.get()
                            + attrib.Offset
                            + attrib.Stride * face[v],
                        attribSize);
                }

                // generate the interpolated data for each edge
                for (std::size_t e = 0; e < edgeData.size(); e++)
                {
                    // interpolate the two attributes into edgeData
                    for (std::size_t i = 0; i < attrib.Cardinality; i++)
                    {
                        void* a1 = vertexData[e].get()
                                 + offsets[e]
                                 + i * SizeOfArithmeticType(attrib.Type);

                        void* a2 = vertexData[(e + 1) % vertexData.size()].get()
                                 + offsets[e]
                                 + i * SizeOfArithmeticType(attrib.Type);

                        void* edge = edgeData[e].get()
                                   + offsets[e]
                                   + i * SizeOfArithmeticType(attrib.Type);

                        if (attrib.Type == ArithmeticType::Float)
                        {
                            float* a1f = static_cast<float*>(a1);
                            float* a2f = static_cast<float*>(a2);
                            float* edgef = static_cast<float*>(edge);

                            *edgef = (*a1f + *a2f) / 2.0f;
                        }
                        else
                        {
                            throw std::runtime_error(
                                "Interpolation not implemented "
                                "for types other than float.");
                        }
                    }

                    offsets[e] += attribSize;
                }
            }

            // now that the data for each edge has been created,
            // can write the subdivided triangles.

            char* cbuffer = static_cast<char*>(buffer);

            // tr1
            std::memcpy(cbuffer + bufferOffset,
                vertexData[0].get(), vertexSize);
            std::memcpy(cbuffer + bufferOffset + vertexSize,
                edgeData[0].get(), vertexSize);
            std::memcpy(cbuffer + bufferOffset + vertexSize * 2,
                edgeData[2].get(), vertexSize);

            bufferOffset += vertexSize * 3;

            // tr2
            std::memcpy(cbuffer + bufferOffset,
                edgeData[0].get(), vertexSize);
            std::memcpy(cbuffer + bufferOffset + vertexSize,
                edgeData[1].get(), vertexSize);
            std::memcpy(cbuffer + bufferOffset + vertexSize * 2,
                edgeData[2].get(), vertexSize);

            bufferOffset += vertexSize * 3;

            // tr3
            std::memcpy(cbuffer + bufferOffset,
                edgeData[0].get(), vertexSize);
            std::memcpy(cbuffer + bufferOffset + vertexSize,
                vertexData[1].get(), vertexSize);
            std::memcpy(cbuffer + bufferOffset + vertexSize * 2,
                edgeData[1].get(), vertexSize);

            bufferOffset += vertexSize * 3;

            // tr4
            std::memcpy(cbuffer + bufferOffset,
                edgeData[2].get(), vertexSize);
            std::memcpy(cbuffer + bufferOffset + vertexSize,
                edgeData[1].get(), vertexSize);
            std::memcpy(cbuffer + bufferOffset + vertexSize * 2,
                vertexData[2].get(), vertexSize);

            bufferOffset += vertexSize * 3;
        }
    }

    return (numBaseIndices > 0 ?
            numBaseIndices : numBaseVertices) * std::pow(4, mNumSubdivisons);
}

std::size_t LoopSubdivisionMesh::WriteIndices(void*) const
{
    return 0;
}


} // end namespace ng
