#ifndef NG_LIGHT_HPP
#define NG_LIGHT_HPP

#include "ng/framework/scenegraph/renderobject.hpp"

#include "ng/engine/math/linearalgebra.hpp"

namespace ng
{

enum LightType
{
    Ambient,
    Point
};

class Light : public IRenderObject
{
    LightType mType;
    vec3 mColor;
    float mRadius;

public:
    Light(LightType type)
        : mType(type)
    { }

    LightType GetLightType() const
    {
        return mType;
    }

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
        if (mType != LightType::Point)
        {
            throw std::logic_error("Only point lights have a radius");
        }

        return mRadius;
    }

    void SetRadius(float radius)
    {
        if (mType != LightType::Point)
        {
            throw std::logic_error("Only point lights have a radius");
        }

        mRadius = radius;
    }

    AxisAlignedBoundingBox<float> GetLocalBoundingBox() const override
    {
        if (mType == LightType::Ambient)
        {
            return {
                vec3(std::numeric_limits<float>::lowest()),
                vec3(std::numeric_limits<float>::max())
            };
        }
        else if (mType == LightType::Point)
        {
            return {
                vec3(-mRadius), vec3(mRadius)
            };
        }
        else
        {
            throw std::logic_error("light type has no local bounding box");
        }
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
