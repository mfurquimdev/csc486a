#include "ng/engine/app.hpp"

#include "ng/engine/quickstart.hpp"

#include "ng/framework/scenegraph/scenegraph.hpp"
#include "ng/framework/scenegraph/renderobjectnode.hpp"
#include "ng/framework/scenegraph/camera.hpp"
#include "ng/framework/scenegraph/light.hpp"
#include "ng/framework/scenegraph/isosurface.hpp"
#include "ng/framework/scenegraph/uvsphere.hpp"
#include "ng/framework/scenegraph/shaderprofile.hpp"

class A2 : public ng::IApp, private ng::QuickStart
{
    ng::ShaderProfileFactory mShaderProfileFactory;

    ng::SceneGraph mROManager;

    std::shared_ptr<ng::RenderObjectNode> mRoot;

    std::shared_ptr<ng::Camera> mCamera;
    std::shared_ptr<ng::CameraNode> mCameraNode;
    ng::vec3 mEyePosition;
    ng::vec3 mEyeTarget;
    ng::vec3 mEyeUpVector;

    ng::Material mStandardMaterial;

    std::shared_ptr<ng::Light> mAmbientLight;
    std::shared_ptr<ng::LightNode> mAmbientLightNode;

    std::shared_ptr<ng::RenderObjectNode> mIsoSurfaceNode;
    std::shared_ptr<ng::IsoSurface> mIsoSurface;

    std::shared_ptr<ng::Light> mIsoSurfaceLight1;
    std::shared_ptr<ng::LightNode> mIsoSurfaceLight1Node;

    std::shared_ptr<ng::Light> mIsoSurfaceLight2;
    std::shared_ptr<ng::LightNode> mIsoSurfaceLight2Node;

    std::chrono::time_point<std::chrono::high_resolution_clock> mStartTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> mNow, mThen;
    std::chrono::milliseconds mLag;
    std::chrono::milliseconds mFixedUpdateStep;

public:
    void Init() override
    {
        mShaderProfileFactory.BuildShaders(Renderer);

        // prepare the scene
        mRoot = std::make_shared<ng::RenderObjectNode>();
        mROManager.SetRoot(mRoot);

        mCamera = std::make_shared<ng::Camera>();
        mCameraNode = std::make_shared<ng::CameraNode>(mCamera);
        mCameraNode->SetPerspective(70.0f, (float) Window->GetWidth() / Window->GetHeight(), 0.1f, 1000.0f);
        mEyePosition = { 10.0f, 10.0f, 10.0f };
        mEyeTarget = { 0.0f, 0.0f, 0.0f };
        mEyeUpVector = { 0.0f, 1.0f, 0.0f };
        mCameraNode->SetLookAt(mEyePosition, mEyeTarget, mEyeUpVector);
        mCameraNode->SetViewport(0, 0, Window->GetWidth(), Window->GetHeight());
        mROManager.SetCamera(mCameraNode);

        mStandardMaterial.Style = ng::MaterialStyle::Phong;

        mAmbientLight = std::make_shared<ng::Light>(ng::LightType::Ambient);
        mAmbientLight->SetColor(ng::vec3(0.1,0.1,0.1));
        mAmbientLightNode = std::make_shared<ng::LightNode>(mAmbientLight);
        mROManager.AddLight(mAmbientLightNode);
        mRoot->AdoptChild(mAmbientLightNode);

        std::vector<ng::IsoPrimitive> primitives;
        primitives.emplace_back(ng::Sphere<float>({0,0,0},0), ng::WyvillFilter(5.0f));
        primitives.emplace_back(ng::Sphere<float>({3,0,0},0), ng::WyvillFilter(3.0f));
        primitives.emplace_back(ng::Sphere<float>({0,3,0},0), ng::WyvillFilter(2.0f));

        mIsoSurface = std::make_shared<ng::IsoSurface>(Renderer);
        mIsoSurface->Polygonize(primitives, 0.3f, 0.1f);

        mIsoSurfaceNode = std::make_shared<ng::RenderObjectNode>();
        mIsoSurfaceNode->SetRenderObject(mIsoSurface);
        mIsoSurfaceNode->SetMaterial(mStandardMaterial);
        mRoot->AdoptChild(mIsoSurfaceNode);

        mIsoSurfaceLight1 = std::make_shared<ng::Light>(ng::LightType::Point);
        mIsoSurfaceLight1->SetColor(ng::vec3(0.7,0.5,0.0));
        mIsoSurfaceLight1->SetRadius(10.0f);
        mIsoSurfaceLight1Node = std::make_shared<ng::LightNode>(mIsoSurfaceLight1);
        mROManager.AddLight(mIsoSurfaceLight1Node);
        mRoot->AdoptChild(mIsoSurfaceLight1Node);
        mIsoSurfaceNode->AdoptChild(mIsoSurfaceLight1Node);

        mIsoSurfaceLight2 = std::make_shared<ng::Light>(ng::LightType::Point);
        mIsoSurfaceLight2->SetColor(ng::vec3(0.0,0.5,0.6));
        mIsoSurfaceLight2->SetRadius(10.0f);
        mIsoSurfaceLight2Node = std::make_shared<ng::LightNode>(mIsoSurfaceLight2);
        mROManager.AddLight(mIsoSurfaceLight2Node);
        mRoot->AdoptChild(mIsoSurfaceLight2Node);
        mIsoSurfaceNode->AdoptChild(mIsoSurfaceLight2Node);

        // do an initial update to get things going
        mROManager.Update(std::chrono::milliseconds(0));

        // initialize time stuff
        mStartTime = std::chrono::high_resolution_clock::now();
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
                mCameraNode->SetPerspective(70.0f, (float) Window->GetWidth() / Window->GetHeight(), 0.1f, 1000.0f);
            }
        }

        {
            std::chrono::duration<float> fsec = mStartTime - mNow;

            float r = 10.0f;
            float x = std::cos(fsec.count() * ng::pi<float>::value);
            float y = std::sin(fsec.count() * ng::pi<float>::value);
            float z = x * y;

            mIsoSurfaceLight1Node->SetLocalTransform(ng::Translate(r * ng::vec3(x,y,z)));
            mIsoSurfaceLight2Node->SetLocalTransform(inverse(mIsoSurfaceLight1Node->GetLocalTransform()));
        }

        Renderer->Clear(true, true, true);

        mROManager.DrawMultiPass(mShaderProfileFactory);

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
