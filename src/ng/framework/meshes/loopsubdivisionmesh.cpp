#include "ng/framework/meshes/loopsubdivisionmesh.hpp"

#include "ng/engine/math/linearalgebra.hpp"
#include "ng/engine/util/debug.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <set>

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

    VertexFormat baseFmt = mMeshToSubdivide->GetVertexFormat();

    if (baseFmt.Position.Enabled == false)
    {
        throw std::logic_error("Cannot subdivide mesh without position");
    }

    if (baseFmt.Position.Cardinality > 3)
    {
        throw std::logic_error(
            "Can only subdivide with position cardinality <= 3");
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
    if (fmt.TexCoord0.Enabled)
    {
        attribs.push_back(fmt.TexCoord0);
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
    VertexFormat baseFmt = mMeshToSubdivide->GetVertexFormat();

    std::unique_ptr<char[]> baseVertices;
    std::size_t numBaseVertices = 0;
    std::size_t maxBaseVertexBufferSize =
        mMeshToSubdivide->GetMaxVertexBufferSize();

    if (maxBaseVertexBufferSize > 0)
    {
        if (buffer != nullptr)
        {
            baseVertices.reset(new char[maxBaseVertexBufferSize]);
        }

        numBaseVertices = mMeshToSubdivide->WriteVertices(baseVertices.get());
    }

    std::unique_ptr<char[]> baseIndices;
    std::size_t numBaseIndices = 0;
    std::size_t maxBaseIndexBufferSize =
        mMeshToSubdivide->GetMaxIndexBufferSize();

    if (maxBaseIndexBufferSize > 0)
    {
        if (buffer != nullptr)
        {
            baseIndices.reset(new char[maxBaseIndexBufferSize]);
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

        // build neighbor map
        struct VectorCompare
        {
            bool operator()(vec3 x, vec3 y) const
            {
                return std::lexicographical_compare(
                    &x[0], &x[0] + 3, &y[0], &y[0] + 3);
            }
        };

        using VectorSet = std::set<vec3,VectorCompare>;
        std::map<vec3,VectorSet,VectorCompare> neighbors;

        for (std::size_t faceIdx = 0; faceIdx < numFaces; faceIdx++)
        {
            sizevec3 face = getFaceVertices(faceIdx);

            std::array<vec3,3> positions;
            for (std::size_t vertexIdx = 0; vertexIdx < 3; vertexIdx++)
            {
                float* posAttrib =
                    reinterpret_cast<float*>(
                        baseVertices.get()
                      + baseFmt.Position.Offset
                      + baseFmt.Position.Stride * face[vertexIdx]);

                for (std::size_t elem = 0;
                     elem < baseFmt.Position.Cardinality && elem < 3;
                     elem++)
                {
                    positions[vertexIdx][elem] = posAttrib[elem];
                }
            }

            for (std::size_t vertexIdx = 0; vertexIdx < 3; vertexIdx++)
            {
                VectorSet& neighborSet =
                    neighbors.emplace(
                        positions[vertexIdx],
                        VectorSet()).first->second;

                for (std::size_t neighborIdx = 0;
                     neighborIdx < 3;
                     neighborIdx++)
                {
                    if (neighborIdx != vertexIdx)
                    {
                        neighborSet.insert(positions[neighborIdx]);
                    }
                }
            }
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
                }

                // alter the original positions if present
                if (&baseFmt.Position == &attrib)
                {
                    for (std::size_t v = 0; v < vertexData.size(); v++)
                    {
                        float* posAttrib =
                            reinterpret_cast<float*>(
                                vertexData[v].get() + offsets[v]);

                        vec3 pos;
                        for (std::size_t elem = 0;
                             elem < attrib.Cardinality && elem < 3;
                             elem++)
                        {
                            pos[elem] = posAttrib[elem];
                        }

                        // look up the position's neighbors
                        const VectorSet& neighborSet = neighbors.at(pos);

                        std::size_t n = neighborSet.size();

                        // calculate B coefficient for weight
                        float B = n == 3 ?
                                   3.0f / 16.0f
                                 : 3.0f / (8.0f * n);

                        vec3 newpos = (1 - B) * pos
                                    + B / n * std::accumulate(
                                                neighborSet.begin(),
                                                neighborSet.end(),
                                                vec3(0));

                        // write back the new position
                        for (std::size_t elem = 0;
                             elem < attrib.Cardinality && elem < 3;
                             elem++)
                        {
                            posAttrib[elem] = newpos[elem];
                        }
                    }
                }

                // update offsets of vertex data
                for (std::size_t e = 0; e < edgeData.size(); e++)
                {
                    offsets[e] += attribSize;
                }
            }

            // now that the data for each vertex and edge has been created,
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
            numBaseIndices : numBaseVertices) * 4;
}

std::size_t LoopSubdivisionMesh::WriteIndices(void*) const
{
    return 0;
}


} // end namespace ng
