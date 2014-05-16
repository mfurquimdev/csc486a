#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"
#include "ng/engine/windowmanager.hpp"
#include "ng/engine/windowevent.hpp"
#include "ng/engine/renderer.hpp"

#include <GL/gl.h>

#include <iostream>

int main()
{
    auto windowManager = ng::CreateWindowManager();
    ng::VideoFlags videoFlags{};
    auto window = windowManager->CreateWindow("test", 640, 480, 0, 0, videoFlags);
    auto renderer = ng::CreateRenderer(windowManager, window);

    while (true)
    {
        ng::WindowEvent e;
        while (windowManager->PollEvent(e))
        {
            if (e.type == ng::WindowEventType::Quit)
            {
                return 0;
            }
            else if (e.type == ng::WindowEventType::MouseMotion)
            {
                std::cout << e.motion.x << ", " << e.motion.y << std::endl;
            }
            else if (e.type == ng::WindowEventType::MouseButton)
            {
                std::cout << e.button.state << " button " << e.button.button
                          << " at (" << e.button.x << ", " << e.button.y << ")"
                          << std::endl;
            }
        }

        // glClear(GL_COLOR_BUFFER_BIT);
        // window->SwapBuffers();
    }
}
