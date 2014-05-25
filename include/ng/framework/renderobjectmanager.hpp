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
class CameraNode;

class RenderObjectManager
{
    std::shared_ptr<CameraNode> mCurrentCamera;

public:
    void SetCurrentCamera(std::shared_ptr<CameraNode> currentCamera);

    const std::shared_ptr<CameraNode>& GetCurrentCamera() const
    {
        return mCurrentCamera;
    }

    // Currently not allowed to modify the scene graph during an update pass.
    // ... but that might be a nice feature for later? Just gotta find a good way to do it.
    void Update(std::chrono::milliseconds deltaTime);

    // Not allowed to modify the scene graph during a draw pass.
    void Draw(const std::shared_ptr<IShaderProgram>& program,
              const RenderState& renderState) const;
};

} // end namespace ng

#endif // NG_RENDEROBJECTMANAGER_HPP
