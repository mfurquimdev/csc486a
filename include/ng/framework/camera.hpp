#ifndef NG_CAMERA_HPP
#define NG_CAMERA_HPP

#include "ng/framework/renderobject.hpp"

#include "ng/engine/linearalgebra.hpp"

namespace ng
{

class PerspectiveView : public IRenderObject
{
    float mFieldOfViewY;
    float mAspectRatio;
    float mNearPlaneDistance;
    float mFarPlaneDistance;

public:
    PerspectiveView()
        : mFieldOfViewY(70.0f)
        , mAspectRatio(4.0f / 3.0f)
        , mNearPlaneDistance(0.1f)
        , mFarPlaneDistance(1000.0f)
    { }

    PerspectiveView(float fieldOfViewY, float aspectRatio,
                    float nearPlaneDistance, float farPlaneDistance)
        : mFieldOfViewY(fieldOfViewY)
        , mAspectRatio(aspectRatio)
        , mNearPlaneDistance(nearPlaneDistance)
        , mFarPlaneDistance(farPlaneDistance)
    { }

    float GetFieldOfViewY() const
    {
        return mFieldOfViewY;
    }

    void SetFieldOfViewY(float fieldOfViewY)
    {
        mFieldOfViewY = fieldOfViewY;
    }

    float GetAspectRatio() const
    {
        return mAspectRatio;
    }

    void SetAspectRatio(float aspectRatio)
    {
        mAspectRatio = aspectRatio;
    }

    float GetNearPlaneDistance() const
    {
        return mNearPlaneDistance;
    }

    void SetNearPlaneDistance(float nearPlaneDistance)
    {
        mNearPlaneDistance = nearPlaneDistance;
    }

    float GetFarPlaneDistance() const
    {
        return mFarPlaneDistance;
    }

    void SetFarPlaneDistance(float nearPlaneDistance)
    {
        mNearPlaneDistance = nearPlaneDistance;
    }

    RenderObjectPass PreUpdate(std::chrono::milliseconds,
                               RenderObjectNode& node) override;

    void PostUpdate(std::chrono::milliseconds,
                    RenderObjectNode&) override
    { }

    RenderObjectPass Draw(
            const std::shared_ptr<IShaderProgram>&,
            const std::map<std::string, UniformValue>&,
            const RenderState&) override
    {
        return RenderObjectPass::Continue;
    }

};

class LookAtCamera : public IRenderObject
{
    vec3 mEyePosition;
    vec3 mCenterPosition;
    vec3 mUpVector;

public:
    LookAtCamera()
        : mEyePosition(0.0f, 0.0f, 1.0f)
        , mCenterPosition(0.0f, 0.0f, 0.0f)
        , mUpVector(0.0f, 1.0f, 0.0f)
    { }

    LookAtCamera(vec3 eyePosition, vec3 centerPosition, vec3 upVector)
        : mEyePosition(eyePosition)
        , mCenterPosition(centerPosition)
        , mUpVector(upVector)
    { }

    vec3 GetEyePosition() const
    {
        return mEyePosition;
    }

    void SetEyePosition(vec3 eyePosition)
    {
        mEyePosition = eyePosition;
    }

    vec3 GetCenterPosition() const
    {
        return mCenterPosition;
    }

    void SetCenterPosition(vec3 centerPosition)
    {
        mCenterPosition = centerPosition;
    }

    vec3 GetUpVector() const
    {
        return mUpVector;
    }

    void SetUpVector(vec3 upVector)
    {
        mUpVector = upVector;
    }

    RenderObjectPass PreUpdate(std::chrono::milliseconds,
                               RenderObjectNode& node) override;

    void PostUpdate(std::chrono::milliseconds,
                    RenderObjectNode&) override
    { }

    RenderObjectPass Draw(
            const std::shared_ptr<IShaderProgram>&,
            const std::map<std::string, UniformValue>&,
            const RenderState&) override
    {
        return RenderObjectPass::Continue;
    }
};

} // end namespace ng

#endif // NG_CAMERA_HPP
