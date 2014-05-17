#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"
#include "ng/engine/windowmanager.hpp"
#include "ng/engine/windowevent.hpp"
#include "ng/engine/renderer.hpp"

#include <GL/gl.h>

#include <iostream>
#include <chrono>

int main()
{
    auto windowManager = ng::CreateWindowManager();
    ng::VideoFlags videoFlags{};
    auto window = windowManager->CreateWindow("test", 640, 480, 0, 0, videoFlags);
    auto renderer = ng::CreateRenderer(windowManager, window);

    std::chrono::time_point<std::chrono::high_resolution_clock> now, then;
    then = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds lag(0);

    while (true)
    {
        now = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - then);
        then = now;
        lag += dt;

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

        while (lag >= std::chrono::milliseconds(1000/60))
        {
            // TODO: update(1000/60 milliseconds)
            lag -= std::chrono::milliseconds(1000/60);
        }

        renderer->Clear(true, true, false);
        renderer->SwapBuffers();
    }
}
