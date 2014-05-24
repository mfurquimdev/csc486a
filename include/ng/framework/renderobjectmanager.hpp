#ifndef NG_RENDEROBJECTMANAGER_HPP
#define NG_RENDEROBJECTMANAGER_HPP

#include "ng/engine/linearalgebra.hpp"

#include <memory>
#include <vector>
#include <chrono>

namespace ng
{

class IRenderObject;
class IShaderProgram;
class RenderState;

class RenderObjectManager
{
public:
    struct ObjectNode
    {
        std::weak_ptr<ObjectNode> ParentNode;
        std::shared_ptr<IRenderObject> Object;
        std::vector<std::shared_ptr<ObjectNode>> ChildNodes;

        mat4 ProjectionTransform;
        mat4 ModelViewTransform;
    };

    const std::vector<std::shared_ptr<ObjectNode>>& GetRoots() const
    {
        return mRootNodes;
    }

    void Update(std::chrono::milliseconds deltaTime);

    void Draw(const std::shared_ptr<IShaderProgram>& program,
              const RenderState& renderState) const;

private:
    std::vector<std::shared_ptr<ObjectNode>> mRootNodes;
};

} // end namespace ng

#endif // NG_RENDEROBJECTMANAGER_HPP
