#ifndef NG_CUBEMESH_HPP
#define NG_CUBEMESH_HPP

#include "ng/framework/renderobject.hpp"

namespace ng
{

class IStaticMesh;
class IRenderer;

class CubeMesh : public IRenderObject
{
    std::shared_ptr<IStaticMesh> mMesh;

    float mSideLength = 0.0f;

public:
    CubeMesh(std::shared_ptr<IRenderer> renderer);

    void Init(float sideLength);

    RenderObjectPass PreUpdate(std::chrono::milliseconds deltaTime,
                               RenderObjectNode& node) override;

    void PostUpdate(std::chrono::milliseconds,
                    RenderObjectNode&) override
    { }

    RenderObjectPass Draw(
            const std::shared_ptr<IShaderProgram>& program,
            const std::map<std::string, UniformValue>& uniforms,
            const RenderState& renderState) override;
};

} // end namespace ng

#endif // NG_CUBEMESH_HPP
