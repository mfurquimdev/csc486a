#include "ng/engine/app.hpp"

#ifdef NG_USE_EMSCRIPTEN
#include <emscripten.h>
#endif

static std::unique_ptr<ng::IApp> gApp;
ng::AppStepAction gAppAction;

static void step()
{
    if (gApp)
    {
        gAppAction = gApp->Step();
    }
    else
    {
        gAppAction = ng::AppStepAction::Quit;
    }
}

int main()
{
    gApp = ng::CreateApp();
    gApp->Init();

#ifdef NG_USE_EMSCRIPTEN
    emscripten_set_main_loop(step, 60, 1);
#else
    while (true)
    {
        step();
        if (gAppAction == ng::AppStepAction::Quit)
        {
            return 0;
        }
    }
#endif
}
