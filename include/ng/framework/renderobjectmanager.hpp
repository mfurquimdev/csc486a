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
struct RenderObjectNode;

class RenderObjectManager
{
public:
    const std::vector<std::shared_ptr<RenderObjectNode>>& GetRoots() const
    {
        return mRootNodes;
    }

    std::shared_ptr<RenderObjectNode> AddRoot();

    void RemoveRoot(std::shared_ptr<RenderObjectNode> node);

    // RenderObject tree currently does not handle being updated during a pass
    // ... but that might be a nice feature for later? Just gotta find a good way to do it.
    // Perhaps using std::set instead of std::vector?
    void Update(std::chrono::milliseconds deltaTime);

    // RenderObject tree may not be modified during drawing.
    void Draw(const std::shared_ptr<IShaderProgram>& program,
              const RenderState& renderState) const;

private:
    std::vector<std::shared_ptr<RenderObjectNode>> mRootNodes;
};

} // end namespace ng

#endif // NG_RENDEROBJECTMANAGER_HPP
