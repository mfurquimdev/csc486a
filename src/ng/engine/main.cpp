#include "ng/engine/app.hpp"
#include "ng/engine/util/debug.hpp"

#ifdef NG_USE_EMSCRIPTEN
#include <emscripten.h>
#endif

static std::shared_ptr<ng::IApp> gApp;
static ng::AppStepAction gAppAction;

static void step() try
{
    if (gApp)
    {
        gAppAction = gApp->Step();
        if (gAppAction == ng::AppStepAction::Quit)
        {
            gApp.reset();
        }
    }
    else
    {
        gAppAction = ng::AppStepAction::Quit;
    }
}
catch (const std::exception& e)
{
    ng::DebugPrintf("Caught top level App Step error:\n%s\n", e.what());
}

int main() try
{
    gApp = ng::CreateApp();
    if (gApp)
    {
        gApp->Init();
    }

#ifdef NG_USE_EMSCRIPTEN
    emscripten_set_main_loop(step, 0, 1);
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
catch (const std::exception& e)
{
    ng::DebugPrintf("Caught top level App Init error:\n%s\n", e.what());
}
