#include "ng/engine/x11/xwindow.hpp"
#include "ng/engine/x11/xglcontext.hpp"

#include "ng/engine/iwindow.hpp"
#include "ng/engine/iglcontext.hpp"

#include <GL/glx.h>

#include <unistd.h>

int main()
{
    const int attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    auto window = ng::CreateXWindow("test",0,0,640,480,attribs);
    auto context = ng::CreateXGLContext(attribs);

    window->MakeCurrent(*context);

    for (int i = 0; i < 5; i++)
    {
        window->SwapBuffers();
        sleep(1);
    }
}
