#include "ng/engine/app.hpp"

#include "ng/engine/quickstart.hpp"

#include "ng/framework/scenegraph/scenegraph.hpp"
#include "ng/framework/scenegraph/renderobjectnode.hpp"
#include "ng/framework/scenegraph/camera.hpp"
#include "ng/framework/scenegraph/isosurface.hpp"
#include "ng/framework/scenegraph/uvsphere.hpp"

class A2 : public ng::IApp, private ng::QuickStart
{
    std::shared_ptr<ng::IShaderProgram> mProgram;
    ng::SceneGraph mROManager;

    std::shared_ptr<ng::Camera> mCamera;
    std::shared_ptr<ng::CameraNode> mCameraNode;

    ng::vec3 mEyePosition;
    ng::vec3 mEyeTarget;
    ng::vec3 mEyeUpVector;

    std::shared_ptr<ng::RenderObjectNode> mIsoSurfaceNode;
    std::shared_ptr<ng::IsoSurface> mIsoSurface;

    std::shared_ptr<ng::RenderObjectNode> mUVSphereNode;
    std::shared_ptr<ng::UVSphere> mUVSphere;

    std::chrono::time_point<std::chrono::high_resolution_clock> mNow, mThen;
    std::chrono::milliseconds mLag;
    std::chrono::milliseconds mFixedUpdateStep;

public:
    void Init() override
    {
        static const char* vsrc = "#version 100\n"
                                  "struct Light {\n"
                                  "    highp vec3 Position;\n"
                                  "    mediump vec4 Color;\n"
                                  "} uLight;\n"
                                  "uniform highp mat4 uModelView;\n"
                                  "uniform highp mat4 uProjection;\n"
                                  "attribute highp vec4 iPosition;\n"
                                  "attribute mediump vec3 iNormal;\n"
                                  "varying highp vec3 fViewPosition;\n"
                                  "varying mediump float fLightTint;\n"
                                  "void main() {\n"
                                  "    fViewPosition = vec3(uModelView * iPosition);\n"
                                  "    fLightTint = min(0.0,dot(iNormal, normalize(uLight.Position - iPosition.xyz)));\n"
                                  "    gl_Position = uProjection * uModelView * iPosition;\n"
                                  "}\n";

        static const char* fsrc = "#version 100\n"
                                  "uniform lowp vec4 uTint;\n"
                                  "varying highp vec3 fViewPosition;\n"
                                  "varying mediump float fLightTint;\n"
                                  "void main() {\n"
                                  "    gl_FragColor = fLightTint * uTint;"
                                  "}\n";

        mProgram = Renderer->CreateShaderProgram();

        mProgram->Init(std::shared_ptr<const char>(vsrc, [](const char*){}),
                       std::shared_ptr<const char>(fsrc, [](const char*){}),
                       true);

        // prepare the scene
        mCamera = std::make_shared<ng::Camera>();
        mCameraNode = std::make_shared<ng::CameraNode>(mCamera);
        mCameraNode->SetPerspective(70.0f, (float) Window->GetWidth() / Window->GetHeight(), 0.1f, 1000.0f);
        mEyePosition = { 10.0f, 10.0f, 10.0f };
        mEyeTarget = { 0.0f, 0.0f, 0.0f };
        mEyeUpVector = { 0.0f, 1.0f, 0.0f };
        mCameraNode->SetLookAt(mEyePosition, mEyeTarget, mEyeUpVector);
        mCameraNode->SetViewport(0, 0, Window->GetWidth(), Window->GetHeight());
        mROManager.SetCamera(mCameraNode);
        mROManager.SetRoot(mCameraNode);

        mIsoSurface = std::make_shared<ng::IsoSurface>(Renderer);
        mIsoSurface->Polygonize([](ng::vec3 p){
            return ng::dot(p,p) - 1;
        }, 1.0f, 1.0f, { ng::ivec3(0,0,0) });

        mIsoSurfaceNode = std::make_shared<ng::RenderObjectNode>();
        mIsoSurfaceNode->SetRenderObject(mIsoSurface);
        mCameraNode->AdoptChild(mIsoSurfaceNode);

        // for testing the camera when the isosurface is glitchy
        mUVSphere = std::make_shared<ng::UVSphere>(Renderer);
        mUVSphere->Init(5, 5, 1.0f);
        mUVSphereNode = std::make_shared<ng::RenderObjectNode>();
        mUVSphereNode->SetRenderObject(mUVSphere);
        // mCameraNode->AdoptChild(mUVSphereNode);

        // do an initial update to get things going
        mROManager.Update(std::chrono::milliseconds(0));

        // initialize time stuff
        mThen = std::chrono::high_resolution_clock::now();
        mLag = std::chrono::milliseconds(0);
        mFixedUpdateStep = std::chrono::milliseconds(1000/60);
    }

    ng::AppStepAction Step() override
    {
        mNow = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds dt = std::chrono::duration_cast<std::chrono::milliseconds>(mNow - mThen);
        mThen = mNow;
        mLag += dt;

        ng::WindowEvent we;
        while (WindowManager->PollEvent(we))
        {
            if (we.Type == ng::WindowEventType::Quit)
            {
                return ng::AppStepAction::Quit;
            }
            else if (we.Type == ng::WindowEventType::MouseMotion)
            {
                if (we.Motion.ButtonStates[int(ng::MouseButton::Right)])
                {
                    int deltaX = we.Motion.X - we.Motion.OldX;
                    float rotation = (float) - deltaX / Window->GetWidth() * ng::pi<float>::value * 2;

                    mEyePosition = ng::vec3(ng::Rotate(rotation, 0.0f, 1.0f, 0.0f) * ng::vec4(mEyePosition, 0.0f));

                    mCameraNode->SetLookAt(mEyePosition, mEyeTarget, mEyeUpVector);
                }
            }
            else if (we.Type == ng::WindowEventType::MouseScroll)
            {
                float zoomDelta = 0.4f;

                mEyePosition = mEyePosition + normalize(mEyePosition - mEyeTarget) * zoomDelta * float(we.Scroll.Delta);

                mCameraNode->SetLookAt(mEyePosition, mEyeTarget, mEyeUpVector);
            }
            else if (we.Type == ng::WindowEventType::WindowStructure)
            {
                mCameraNode->SetViewport(0, 0, Window->GetWidth(), Window->GetHeight());
            }
        }

        Renderer->Clear(true, true, true);

        mROManager.DrawMultiPass(mProgram, ng::RenderState());

        Renderer->SwapBuffers();

        return ng::AppStepAction::Continue;
    }
};

namespace ng
{

std::shared_ptr<ng::IApp> CreateApp()
{
    return std::shared_ptr<ng::IApp>(new A2());
}

} // end namespace ng
