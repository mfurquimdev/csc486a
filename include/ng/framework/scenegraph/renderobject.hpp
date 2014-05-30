#ifndef NG_RENDEROBJECT_HPP
#define NG_RENDEROBJECT_HPP

#include "ng/engine/math/geometry.hpp"

#include <chrono>
#include <map>
#include <string>
#include <memory>

namespace ng
{

class IShaderProgram;
class UniformValue;
class RenderState;
class RenderObjectNode;

enum class RenderObjectPass
{
    Continue,
    SkipChildren
};

class IRenderObject
{
public:
    virtual ~IRenderObject() = default;

    virtual AxisAlignedBoundingBox<float> GetLocalBoundingBox() const = 0;

    virtual RenderObjectPass PreUpdate(std::chrono::milliseconds deltaTime,
                                       RenderObjectNode& node) = 0;

    virtual void PostUpdate(std::chrono::milliseconds deltaTime,
                            RenderObjectNode& node) = 0;

    virtual RenderObjectPass Draw(
            const std::shared_ptr<IShaderProgram>& program,
            const std::map<std::string, UniformValue>& uniforms,
            const RenderState& renderState) = 0;
};

} // end namespace ng

#endif // NG_RENDEROBJECT_HPP
