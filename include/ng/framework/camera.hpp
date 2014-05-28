#ifndef NG_CAMERA_HPP
#define NG_CAMERA_HPP

#include "ng/framework/renderobjectnode.hpp"
#include "ng/framework/renderobject.hpp"

#include "ng/engine/linearalgebra.hpp"

namespace ng
{

class RenderObjectManagerCameraHelper
{
protected:
    bool mIsCurrentCamera;

public:
    friend class RenderObjectManager;
};

class Camera : public RenderObjectManagerCameraHelper, public IRenderObject
{
    int mTimesUpdated = 0;

public:
    AxisAlignedBoundingBox<float> GetLocalBoundingBox() const override
    {
        return AxisAlignedBoundingBox<float>();
    }

    RenderObjectPass PreUpdate(std::chrono::milliseconds,
                               RenderObjectNode&) override
    {
        mTimesUpdated++;

        if (!mIsCurrentCamera || mTimesUpdated > 1)
        {
            return RenderObjectPass::SkipChildren;
        }

        return RenderObjectPass::Continue;
    }

    void PostUpdate(std::chrono::milliseconds,
                    RenderObjectNode&) override
    {
        mTimesUpdated--;
    }

    RenderObjectPass Draw(const std::shared_ptr<IShaderProgram>&,
                          const std::map<std::string, UniformValue>&,
                          const RenderState&) override
    {
        return RenderObjectPass::SkipChildren;
    }
};

class CameraNode : public RenderObjectNode
{
    std::shared_ptr<ng::Camera> mCamera;

    mat4 mProjection;
    float mZFar = 0.0f;
    float mZNear = 0.0f;

    ivec4 mViewport;

protected:
    bool IsCamera() override
    {
        return true;
    }

public:
    CameraNode(std::shared_ptr<ng::Camera> camera)
    {
        SetCamera(camera);
    }

    const std::shared_ptr<ng::Camera>& GetCamera() const
    {
        return mCamera;
    }

    void SetCamera(std::shared_ptr<ng::Camera> camera)
    {
        mCamera = camera;
        RenderObjectNode::SetRenderObject(camera);
    }

    void SetRenderObject(std::shared_ptr<IRenderObject>) override
    {
        throw std::logic_error("May not call RenderObjectNode::SetRenderObject on CameraNode");
    }

    mat4 GetProjection() const
    {
        return mProjection;
    }

    void SetPerspectiveProjection(float fovy, float aspect, float zNear, float zFar)
    {
        mZNear = zNear;
        mZFar = zFar;
        mProjection = Perspective(fovy, aspect, zNear, zFar);
    }

    float GetZNear() const
    {
        return mZNear;
    }

    float GetZFar() const
    {
        return mZFar;
    }

    void SetLookAt(vec3 eye, vec3 center, vec3 up)
    {
        SetLocalTransform(inverse(LookAt(eye, center, up)));
    }

    ivec4 GetViewport() const
    {
        return mViewport;
    }

    void SetViewport(int x, int y, int w, int h)
    {
        SetViewport({x,y,w,h});
    }

    void SetViewport(ivec4 viewport)
    {
        mViewport = viewport;
    }
};

} // end namespace ng

#endif // NG_CAMERA_HPP
