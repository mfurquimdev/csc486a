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

class IRenderObject
{
public:
    virtual ~IRenderObject() = default;

    virtual void Update(std::chrono::milliseconds deltaTime) = 0;

    virtual void Draw(
            const std::shared_ptr<IShaderProgram>& program,
            const std::map<std::string, UniformValue>& uniforms,
            const RenderState& renderState) = 0;
};

} // end namespace ng

#endif // NG_RENDEROBJECT_HPP
