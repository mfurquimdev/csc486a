#include "ng/engine/app.hpp"

#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/window.hpp"
#include "ng/engine/window/windowevent.hpp"
#include "ng/engine/rendering/renderer.hpp"

#include "ng/engine/util/memory.hpp"
#include "ng/engine/util/scopeguard.hpp"

#include "ng/engine/rendering/scenegraph.hpp"
#include "ng/framework/renderobjects/cubemesh.hpp"

#include <vector>

namespace a4
{

class A4 : public ng::IApp
{
    std::shared_ptr<ng::IWindowManager> mWindowManager;
    std::shared_ptr<ng::IWindow> mWindow;
    std::shared_ptr<ng::IRenderer> mRenderer;

    ng::SceneGraph mScene;
    std::shared_ptr<ng::SceneGraphCameraNode> mMainCamera;

public:
    void Init() override
    {
        mWindowManager = ng::CreateWindowManager();
        mWindow = mWindowManager->CreateWindow("a4", 640, 480, 0, 0, ng::VideoFlags());
        mRenderer = ng::CreateRenderer(mWindowManager, mWindow);

        std::shared_ptr<ng::SceneGraphNode> rootNode =
                    std::make_shared<ng::SceneGraphNode>();
        mScene.Root = rootNode;

        std::shared_ptr<ng::SceneGraphNode> cubeNode =
                    std::make_shared<ng::SceneGraphNode>();

        cubeNode->Mesh = std::make_shared<ng::CubeMesh>(1.0f);
        rootNode->Children.push_back(cubeNode);

        mMainCamera = std::make_shared<ng::SceneGraphCameraNode>();
        rootNode->Children.push_back(mMainCamera);
        mScene.ActiveCameras.push_back(mMainCamera);
        mMainCamera->Transform = ng::Translate(0.0f,0.0f,3.0f);
    }

    ng::AppStepAction Step() override
    {
        ng::WindowEvent we;
        while (mWindowManager->PollEvent(we))
        {
            if (we.Type == ng::WindowEventType::Quit)
            {
                return ng::AppStepAction::Quit;
            }
        }

        mMainCamera->Projection =
            ng::Perspective(
                    70.0f,
                    mWindow->GetAspect(),
                    0.1f, 1000.0f);
        mMainCamera->ViewportTopLeft = ng::ivec2(0,0);
        mMainCamera->ViewportSize = ng::ivec2(
                mWindow->GetWidth(), mWindow->GetHeight());

        {
            mRenderer->BeginFrame();
            auto endFrameScope = ng::make_scope_guard([&]{
                mRenderer->EndFrame();
            });

            mRenderer->Render(mScene);
        }

        return ng::AppStepAction::Continue;
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
