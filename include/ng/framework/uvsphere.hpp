#ifndef NG_UVSPHERE_HPP
#define NG_UVSPHERE_HPP

#include "ng/framework/renderobject.hpp"

namespace ng
{

class IRenderer;
class IStaticMesh;

class UVSphere : public IRenderObject
{
    std::shared_ptr<IStaticMesh> mMesh;

    int mNumRings = 0;
    int mNumSegments = 0;
    float mRadius;

public:
    UVSphere(std::shared_ptr<IRenderer> renderer);

    void Init(int numRings, int numSegments, float radius);

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

#endif // NG_UVSPHERE_HPP
