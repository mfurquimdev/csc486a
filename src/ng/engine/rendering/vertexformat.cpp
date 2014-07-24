#include "ng/engine/rendering/vertexformat.hpp"

namespace ng
{

std::array<std::reference_wrapper<const VertexAttribute>,5>
    GetAttribArray(const VertexFormat& fmt)
{
    return {
      std::ref(fmt.Position), std::ref(fmt.Normal), std::ref(fmt.TexCoord0),
      std::ref(fmt.JointIndices), std::ref(fmt.JointWeights)
    };
}

std::array<std::reference_wrapper<VertexAttribute>,5>
    GetAttribArray(VertexFormat& fmt)
{
    return {
      std::ref(fmt.Position), std::ref(fmt.Normal), std::ref(fmt.TexCoord0),
      std::ref(fmt.JointIndices), std::ref(fmt.JointWeights)
    };
}

} // end namespace ng
