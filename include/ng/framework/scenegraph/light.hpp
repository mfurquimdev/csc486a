#ifndef NG_LIGHT_HPP
#define NG_LIGHT_HPP

#include "ng/framework/scenegraph/renderobject.hpp"

#include "ng/engine/math/linearalgebra.hpp"

namespace ng
{

class Light : public IRenderObject
{
    vec3 mColor;
    float mRadius;

public:
    vec3 GetColor() const
    {
        return mColor;
    }

    void SetColor(vec3 color)
    {
        mColor = color;
    }

    float GetRadius() const
    {
        return mRadius;
    }

    void SetRadius(float radius)
    {
        mRadius = radius;
    }

    AxisAlignedBoundingBox<float> GetLocalBoundingBox() const override
    {
        return {
            vec3(-mRadius), vec3(mRadius)
        };
    }

    RenderObjectPass PreUpdate(std::chrono::milliseconds,
                               RenderObjectNode&) override
    {
        return RenderObjectPass::Continue;
    }

    void PostUpdate(std::chrono::milliseconds,
                    RenderObjectNode&) override
    { }

    RenderObjectPass Draw(const std::shared_ptr<IShaderProgram>&,
                          const std::map<std::string, UniformValue>&,
                          const RenderState&) override
    {
        return RenderObjectPass::Continue;
    }
};

class LightNode : public RenderObjectNode
{
    std::shared_ptr<Light> mLight;

protected:
    bool IsLight() override
    {
        return true;
    }

public:
    LightNode(std::shared_ptr<Light> light)
    {
        SetLight(std::move(light));
    }

    const std::shared_ptr<Light>& GetLight() const
    {
        return mLight;
    }

    void SetLight(std::shared_ptr<Light> light)
    {
        mLight = light;
        RenderObjectNode::SetRenderObject(light);
    }

    void SetRenderObject(std::shared_ptr<IRenderObject>) override
    {
        throw std::logic_error("May not call RenderObjectNode::SetRenderObject on LightNode");
    }
};

} // end namespace ng

#endif // NG_LIGHT_HPP
