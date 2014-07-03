#ifndef NG_SCENEGRAPH_HPP
#define NG_SCENEGRAPH_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <vector>
#include <memory>

namespace ng
{

class IMesh;

class SceneGraphNode
{
public:
    mat4 Transform;
    std::shared_ptr<IMesh> Mesh;
    std::vector<std::shared_ptr<SceneGraphNode>> Children;
};

class SceneGraph
{
public:
    std::shared_ptr<SceneGraphNode> Root;
};

} // end namespace ng

#endif // NG_SCENEGRAPH_HPP
