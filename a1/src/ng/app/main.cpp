#include "ng/engine/x11/xwindow.hpp"
#include "ng/engine/x11/xglcontext.hpp"

#include "ng/engine/iwindow.hpp"
#include "ng/engine/iglcontext.hpp"

#include <GL/glx.h>

#include <unistd.h>

int main()
{
    const int attribs[] =
    {
      GLX_X_RENDERABLE    , True,
      GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
      GLX_RENDER_TYPE     , GLX_RGBA_BIT,
      GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
      GLX_RED_SIZE        , 8,
      GLX_GREEN_SIZE      , 8,
      GLX_BLUE_SIZE       , 8,
      GLX_ALPHA_SIZE      , 8,
      GLX_DEPTH_SIZE      , 24,
      GLX_STENCIL_SIZE    , 8,
      GLX_DOUBLEBUFFER    , True,
      //GLX_SAMPLE_BUFFERS  , 1,
      //GLX_SAMPLES         , 4,
      None
    };

    auto window = ng::CreateXWindow("test",0,0,640,480,attribs);
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
