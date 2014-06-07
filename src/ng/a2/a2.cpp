#include "ng/engine/app.hpp"

#include "ng/engine/quickstart.hpp"

class A2 : public ng::IApp, private ng::QuickStart
{

public:
    void Init() override
    {
    }

    ng::AppStepAction Step() override
    {
        ng::WindowEvent we;
        while (WindowManager->PollEvent(we))
        {
            if (we.Type == ng::WindowEventType::Quit)
            {
                return ng::AppStepAction::Quit;
            }
        }

        Renderer->Clear(true, true, true);

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
