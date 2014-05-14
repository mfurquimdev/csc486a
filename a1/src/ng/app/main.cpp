#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"

#include <GL/gl.h>
#include <unistd.h>

int main()
{
    auto window = ng::CreateWindow("test", 640, 480);
    auto context = window->CreateContext();

    window->MakeCurrent(*context);

    for (int i = 0; i < 5; i++)
    {
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        window->SwapBuffers();
        sleep(1);
    }
}
