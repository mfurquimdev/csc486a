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
#include "ng/framework/meshes/squaremesh.hpp"

#include "ng/framework/textures/checkerboardtexture.hpp"

#include "ng/framework/util/fixedstepupdate.hpp"

#include <vector>
#include <chrono>

namespace a4
{

class A4 : public ng::IApp
{
    std::shared_ptr<ng::IWindowManager> mWindowManager;
    std::shared_ptr<ng::IWindow> mWindow;
    std::shared_ptr<ng::IRenderer> mRenderer;

    ng::SceneGraph mScene;
    std::shared_ptr<ng::SceneGraphCameraNode> mMainCamera;

    std::shared_ptr<ng::SceneGraphCameraNode> mOverlayCamera;

    ng::FixedStepUpdate mFixedStepUpdate{std::chrono::milliseconds(1000/60)};

public:
    void Init() override
    {
        mWindowManager = ng::CreateWindowManager();

        mWindow = mWindowManager->CreateWindow(
            "a4", 640, 480, 0, 0, ng::VideoFlags());

        mRenderer = ng::CreateRenderer(mWindowManager, mWindow);

        // setup materials
        ng::Material normalColoredMaterial(ng::MaterialType::NormalColored);

        ng::Material checkeredMaterial(ng::MaterialType::Textured);
        checkeredMaterial.Texture0 =
            std::make_shared<ng::CheckerboardTexture>(
                2, 2, 1,
                ng::vec4(1), ng::vec4(0));
        checkeredMaterial.Sampler0.MinFilter = ng::TextureFilter::Nearest;
        checkeredMaterial.Sampler0.MagFilter = ng::TextureFilter::Nearest;
        checkeredMaterial.Sampler0.WrapX = ng::TextureWrap::ClampToEdge;
        checkeredMaterial.Sampler0.WrapY = ng::TextureWrap::ClampToEdge;

        // setup scene
        std::shared_ptr<ng::SceneGraphNode> rootNode =
                std::make_shared<ng::SceneGraphNode>();
        mScene.Root = rootNode;

        std::shared_ptr<ng::SceneGraphNode> cubeNode =
                std::make_shared<ng::SceneGraphNode>();

        cubeNode->Mesh = std::make_shared<ng::CubeMesh>(1.0f);
        cubeNode->Material = normalColoredMaterial;
        rootNode->Children.push_back(cubeNode);

        mMainCamera = std::make_shared<ng::SceneGraphCameraNode>();
        rootNode->Children.push_back(mMainCamera);
        mScene.ActiveCameras.push_back(mMainCamera);

        // setup overlay
        std::shared_ptr<ng::SceneGraphNode> overlayRootNode =
                std::make_shared<ng::SceneGraphNode>();
        mScene.OverlayRoot = overlayRootNode;

        std::shared_ptr<ng::SceneGraphNode> squareNode =
                std::make_shared<ng::SceneGraphNode>();

        squareNode->Mesh = std::make_shared<ng::SquareMesh>(100.0f);
        squareNode->Material = checkeredMaterial;
        squareNode->Transform = ng::translate(100.0f, 100.0f, 0.0f);
        overlayRootNode->Children.push_back(squareNode);

        mOverlayCamera = std::make_shared<ng::SceneGraphCameraNode>();
        overlayRootNode->Children.push_back(mOverlayCamera);
        mScene.OverlayActiveCameras.push_back(mOverlayCamera);
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
    ng::vec3 mCameraPosition{4.0f,4.0f,4.0f};
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

        mOverlayCamera->Projection =
                ng::ortho2D(0.0f, (float) mWindow->GetWidth(),
                            0.0f, (float) mWindow->GetHeight());
        mOverlayCamera->ViewportTopLeft - ng::ivec2(0,0);
        mOverlayCamera->ViewportSize = ng::ivec2(
                    mWindow->GetWidth(), mWindow->GetHeight());
    }

    void UpdateCameraTransform(std::chrono::milliseconds dt)
    {
        dt = std::chrono::milliseconds(0);

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

} // end namespace a4

namespace ng
{

std::shared_ptr<IApp> CreateApp()
{
    return std::shared_ptr<IApp>(new a4::A4());
}

} // end namespace ng
