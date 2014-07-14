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

#include "ng/framework/util/fixedstepupdate.hpp"

#include <vector>

class A3 : public ng::IApp
{
    std::shared_ptr<ng::IWindowManager> mWindowManager;
    std::shared_ptr<ng::IWindow> mWindow;
    std::shared_ptr<ng::IRenderer> mRenderer;

    ng::SceneGraph mScene;
    std::shared_ptr<ng::SceneGraphCameraNode> mMainCamera;

    ng::FixedStepUpdate mFixedStepUpdate{std::chrono::milliseconds(1000/60)};

public:
    void Init() override
    {
        mWindowManager = ng::CreateWindowManager();
        mWindow = mWindowManager->CreateWindow("a4", 640, 480, 0, 0, ng::VideoFlags());
        mRenderer = ng::CreateRenderer(mWindowManager, mWindow);

        // setup materials
//        std::shared_ptr<ng::Material> normalColoredMaterial =
//                std::make_shared<ng::Material>();
//        normalColoredMaterial->Type = ng::MaterialType::NormalColored;
        std::shared_ptr<ng::Material> wireframeMaterial =
                std::make_shared<ng::Material>();
        wireframeMaterial->Type = ng::MaterialType::Wireframe;

        // setup scene
        std::shared_ptr<ng::SceneGraphNode> rootNode =
                std::make_shared<ng::SceneGraphNode>();
        mScene.Root = rootNode;

        std::shared_ptr<ng::SceneGraphNode> cubeNode =
                std::make_shared<ng::SceneGraphNode>();

        cubeNode->Mesh = std::make_shared<ng::CubeMesh>(1.0f);
        cubeNode->Mesh =
                std::make_shared<ng::LoopSubdivisionMesh>(cubeNode->Mesh, 1);
        cubeNode->Material = wireframeMaterial;
        // cubeNode->Material = normalColoredMaterial;
        rootNode->Children.push_back(cubeNode);

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
    ng::vec3 mCameraPosition{3.0f,3.0f,3.0f};
    ng::vec3 mCameraTarget{0.0f,0.0f,0.0f};

    void HandleEvent(const ng::WindowEvent&)
    {

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
        mCameraPosition = ng::vec3(ng::rotate(3.14f * dt.count() / 1000,
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
