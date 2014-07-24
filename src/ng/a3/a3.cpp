#include "ng/engine/app.hpp"

#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/window.hpp"
#include "ng/engine/window/windowevent.hpp"
#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/scenegraph.hpp"
#include "ng/engine/rendering/material.hpp"

#include "ng/engine/math/constants.hpp"

#include "ng/engine/util/memory.hpp"
#include "ng/engine/util/scopeguard.hpp"

#include "ng/framework/meshes/cubemesh.hpp"
#include "ng/framework/meshes/loopsubdivisionmesh.hpp"
#include "ng/framework/meshes/implicitsurfacemesh.hpp"

#include "ng/engine/filesystem/filesystem.hpp"
#include "ng/framework/loaders/objloader.hpp"
#include "ng/framework/models/objmodel.hpp"
#include "ng/framework/meshes/objmesh.hpp"

#include "ng/framework/util/fixedstepupdate.hpp"

#include <vector>

class A3 : public ng::IApp
{
    std::shared_ptr<ng::IFileSystem> mFileSystem;

    std::shared_ptr<ng::IWindowManager> mWindowManager;
    std::shared_ptr<ng::IWindow> mWindow;
    std::shared_ptr<ng::IRenderer> mRenderer;

    ng::SceneGraph mScene;
    std::shared_ptr<ng::SceneGraphCameraNode> mMainCamera;

    ng::FixedStepUpdate mFixedStepUpdate{std::chrono::milliseconds(1000/60)};

    std::shared_ptr<ng::SceneGraphNode> mCubeNode;
    std::vector<std::shared_ptr<ng::IMesh>> mCubeSubdivisionStack;

public:
    void Init() override
    {
        mFileSystem = ng::CreateFileSystem();

        mWindowManager = ng::CreateWindowManager();
        mWindow = mWindowManager->CreateWindow("a3", 640, 480, 0, 0, ng::VideoFlags());
        mRenderer = ng::CreateRenderer(mWindowManager, mWindow);

        // setup materials
        ng::Material normalColoredMaterial(ng::MaterialType::NormalColored);

        ng::Material wireframeMaterial(ng::MaterialType::Wireframe);

        // setup scene
        std::shared_ptr<ng::SceneGraphNode> rootNode =
                std::make_shared<ng::SceneGraphNode>();
        mScene.Root = rootNode;

        mCubeNode = std::make_shared<ng::SceneGraphNode>();

//        std::vector<ng::ImplicitSurfacePrimitive> implicitPrimitives;

//        implicitPrimitives.emplace_back(
//                    ng::Point<float>({0,0,0}), ng::WyvillFilter(5.0f));

//        mCubeNode->Mesh = std::make_shared<ng::ImplicitSurfaceMesh>(
//                    std::move(implicitPrimitives),
//                    0.3f,
//                    2.0f);

//         mCubeNode->Mesh = std::make_shared<ng::CubeMesh>(1.0f);
        std::shared_ptr<ng::IReadFile> bunnyFile = mFileSystem->GetReadFile("donut.obj", ng::FileReadMode::Text);
        ng::ObjModel model;
        ng::LoadObj(model, *bunnyFile);
        mCubeNode->Mesh = std::make_shared<ng::ObjMesh>(std::move(model));
        mCubeNode->Material = wireframeMaterial;
        rootNode->Children.push_back(mCubeNode);

        mCubeSubdivisionStack.push_back(mCubeNode->Mesh);

        mMainCamera = std::make_shared<ng::SceneGraphCameraNode>();
        rootNode->Children.push_back(mMainCamera);
        mScene.ActiveCameras.push_back(mMainCamera);
    }

    ng::AppStepAction Step() override
    {
        mFixedStepUpdate.QueuePendingSteps();

        while (mFixedStepUpdate.GetNumPendingSteps() > 0)
        {
            ng::WindowEvent we;
            while (mWindowManager->PollEvent(we))
            {
                if (we.Type == ng::WindowEventType::Quit)
                {
                    return ng::AppStepAction::Quit;
                }
                else
                {
                    HandleEvent(we);
                }
            }

            Update(mFixedStepUpdate.GetStepDuration());
            mFixedStepUpdate.Step();
        }

        {
            const ng::vec3 cornflowerBlue(
                        100.0f / 255.0f,
                        149.0f / 255.0f,
                        237.0f / 255.0f);

            mRenderer->BeginFrame(cornflowerBlue);
            auto endFrameScope = ng::make_scope_guard([&]{
                mRenderer->EndFrame();
            });

            mRenderer->Render(mScene);
        }

        return ng::AppStepAction::Continue;
    }

private:
    ng::vec3 mCameraPosition{2.0f};
    ng::vec3 mCameraTarget{0.0f,0.0f,0.0f};

    void HandleEvent(const ng::WindowEvent& we)
    {
        if (we.Type == ng::WindowEventType::KeyPress)
        {
            if (we.KeyPress.Scancode == ng::Scancode::UpArrow)
            {
                std::shared_ptr<ng::IMesh> curr = mCubeSubdivisionStack.back();
                mCubeSubdivisionStack.push_back(
                    std::make_shared<ng::LoopSubdivisionMesh>(curr));
                mCubeNode->Mesh = mCubeSubdivisionStack.back();
            }
            else if (we.KeyPress.Scancode == ng::Scancode::DownArrow)
            {
                if (mCubeSubdivisionStack.size() > 1)
                {
                    mCubeSubdivisionStack.pop_back();
                    mCubeNode->Mesh = mCubeSubdivisionStack.back();
                }
            }
        }
    }

    void UpdateCameraToWindow()
    {
        mMainCamera->Projection =
                ng::perspective(
                    70.0f,
                    mWindow->GetAspect(),
                    0.1f, 1000.0f);

        mMainCamera->ViewportTopLeft = ng::ivec2(0,0);

        mMainCamera->ViewportSize = ng::ivec2(
                    mWindow->GetWidth(), mWindow->GetHeight());
    }

    void UpdateCameraTransform(std::chrono::milliseconds dt)
    {
        // dt = std::chrono::milliseconds(0);

        mCameraPosition = ng::vec3(ng::rotate4x4(3.14f * dt.count() / 1000,
                                              0.0f, 1.0f, 0.0f)
                                 * ng::vec4(mCameraPosition,1.0f));

        mMainCamera->Transform = inverse(ng::lookAt(mCameraPosition,
                                                    mCameraTarget,
                                                    ng::vec3(0.0f,1.0f,0.0f)));
    }

    void Update(std::chrono::milliseconds dt)
    {
        UpdateCameraToWindow();
        UpdateCameraTransform(dt);
    }
};

namespace ng
{

std::shared_ptr<IApp> CreateApp()
{
    return std::shared_ptr<IApp>(new A3());
}

} // end namespace ng
