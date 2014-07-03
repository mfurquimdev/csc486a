#include "ng/engine/app.hpp"

#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/window.hpp"
#include "ng/engine/window/windowevent.hpp"
#include "ng/engine/rendering/renderer.hpp"

#include "ng/engine/util/memory.hpp"

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

public:
    void Init() override
    {
        mWindowManager = ng::CreateWindowManager();
        mWindow = mWindowManager->CreateWindow("a4", 640, 480, 0, 0, ng::VideoFlags());
        mRenderer = ng::CreateRenderer(mWindowManager, mWindow);

        std::shared_ptr<ng::SceneGraphNode> root =
                           std::make_shared<ng::SceneGraphNode>();

        root->Mesh = std::make_shared<ng::CubeMesh>(1.0f);

        mScene.Root = std::move(root);
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

        mRenderer->BeginFrame();
        mRenderer->Render(mScene);
        mRenderer->EndFrame();

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
