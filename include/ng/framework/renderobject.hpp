#ifndef NG_RENDEROBJECT_HPP
#define NG_RENDEROBJECT_HPP

#include <chrono>
#include <map>
#include <string>
#include <memory>

namespace ng
{

class IShaderProgram;
class UniformValue;
class RenderState;
struct RenderObjectNode;

enum class RenderObjectPass
{
    Continue,
    SkipChildren
};

class IRenderObject
{
public:
    virtual ~IRenderObject() = default;

    // return true if you want to "eat" the update, which means that children won't be updated.
    // return false if you want the children to also be updated.
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
