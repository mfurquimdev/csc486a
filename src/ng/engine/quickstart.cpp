#include "ng/engine/quickstart.hpp"

namespace ng
{

QuickStart::QuickStart(int width, int height, const char* title)
{
    WindowManager = CreateWindowManager();
    Window = WindowManager->CreateWindow(title, width, height, 0, 0, VideoFlags());
    Renderer = CreateRenderer(WindowManager, Window);
}

} // end namespace ng
