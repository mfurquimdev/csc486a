#ifndef NG_OPENGLES2COMMANDVISITOR_HPP
#define NG_OPENGLES2COMMANDVISITOR_HPP

#include "ng/engine/opengl/openglcommands.hpp"

#include "ng/engine/window/window.hpp"
#include "ng/engine/window/glcontext.hpp"

#include <GL/gl.h>

namespace ng
{

class OpenGLES2CommandVisitor : public IRendererCommandVisitor
{
    std::shared_ptr<IGLContext> mGLContext;
    std::shared_ptr<IWindow> mWindow;

    bool mShouldQuit = false;

public:
    OpenGLES2CommandVisitor(
            std::shared_ptr<IGLContext> context,
            std::shared_ptr<IWindow> window)
        : mGLContext(std::move(context))
        , mWindow(std::move(window))
    { }

    void Visit(BeginFrameCommand&) override
    {
        glClear(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);
    }

    void Visit(EndFrameCommand&) override
    {
        mWindow->SwapBuffers();
    }

    void Visit(RenderObjectsCommand&) override
    {

    }

    void Visit(QuitCommand&) override
    {
        mShouldQuit = true;
    }

    bool ShouldQuit() override
    {
        return mShouldQuit;
    }
};

} // end namespace ng

#endif // NG_OPENGLES2COMMANDVISITOR_HPP
