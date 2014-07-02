#include "ng/engine/app.hpp"

#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/window.hpp"
#include "ng/engine/window/windowevent.hpp"
#include "ng/engine/rendering/renderer.hpp"

#include "ng/engine/util/memory.hpp"

#include "ng/engine/rendering/renderobject.hpp"
#include "ng/framework/renderobjects/cubemesh.hpp"

#include <vector>

namespace a4
{

class A4 : public ng::IApp
{
    std::shared_ptr<ng::IWindowManager> mWindowManager;
    std::shared_ptr<ng::IWindow> mWindow;
    std::shared_ptr<ng::IRenderer> mRenderer;

public:
    void Init() override
    {
        mWindowManager = ng::CreateWindowManager();
        mWindow = mWindowManager->CreateWindow("a4", 640, 480, 0, 0, ng::VideoFlags());
        mRenderer = ng::CreateRenderer(mWindowManager, mWindow);
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

        std::unique_ptr<ng::RenderObject[]> renderObjects;
        renderObjects.reset(new ng::RenderObject[1]);

        renderObjects[0].Mesh = std::make_shared<ng::CubeMesh>(1.0f);

        mRenderer->BeginFrame();
        mRenderer->Render(std::move(renderObjects), 1);
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
