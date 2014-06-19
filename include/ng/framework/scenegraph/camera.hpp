#ifndef NG_CAMERA_HPP
#define NG_CAMERA_HPP

#include "ng/framework/scenegraph/renderobjectnode.hpp"
#include "ng/framework/scenegraph/renderobject.hpp"

#include "ng/engine/math/linearalgebra.hpp"

namespace ng
{

class Camera : public IRenderObject
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

class CameraNode : public RenderObjectNode
{
    std::shared_ptr<Camera> mCamera;

    mat4 mProjection;
    mat4 mWorldView;

    float mZFar = 0.0f;
    float mZNear = 0.0f;

    ivec4 mViewport;

public:
    CameraNode(std::shared_ptr<Camera> camera)
    {
        SetCamera(std::move(camera));
    }

    const std::shared_ptr<Camera>& GetCamera() const
    {
        return mCamera;
    }

    void SetCamera(std::shared_ptr<Camera> camera)
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

    void SetPerspective(float fovy, float aspect, float zNear, float zFar)
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

    mat4 GetWorldView() const
    {
        return mWorldView;
    }

    void SetLookAt(vec3 eye, vec3 center, vec3 up)
    {
        mWorldView = LookAt(eye, center, up);

        vec3 z = normalize(eye - center);
        vec3 x = cross(normalize(up), z);
        vec3 y = cross(z,x);

        SetLocalTransform(mat4(vec4(x,0), vec4(y,0), vec4(z,0), vec4(eye,1)));
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
