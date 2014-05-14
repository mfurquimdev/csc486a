#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"
#include "ng/engine/windowmanager.hpp"
#include "ng/engine/windowevent.hpp"

#include <GL/gl.h>
#include <unistd.h>

#include <iostream>

int main()
{
    auto windowManager = ng::CreateWindowManager();
    auto window = windowManager->CreateWindow("test", 640, 480);
    auto context = window->CreateContext();

    window->MakeCurrent(*context);

    while (true)
    {
        ng::WindowEvent e;
        while (windowManager->PollEvent(e))
        {
            if (e.type == ng::WindowEventType::Quit)
            {
                return 0;
            }
            else if (e.type == ng::WindowEventType::Motion)
            {
                std::cout << e.motion.x << ", " << e.motion.y << std::endl;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        window->SwapBuffers();
    }
}
