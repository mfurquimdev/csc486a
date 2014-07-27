#ifndef NG_OPENGLES2COMMANDVISITOR_HPP
#define NG_OPENGLES2COMMANDVISITOR_HPP

#include "ng/engine/opengl/openglcommands.hpp"

#include <GL/gl.h>

#include <functional>

namespace ng
{

class IGLContext;
class IWindow;

class OpenGLES2CommandVisitor : public IRendererCommandVisitor
{
    IGLContext* mGLContext;
    IWindow* mWindow;

    bool mShouldQuit = false;

    PFNGLGENBUFFERSPROC glGenBuffers;
    PFNGLDELETEBUFFERSPROC glDeleteBuffers;
    PFNGLBINDBUFFERPROC glBindBuffer;
    PFNGLBUFFERDATAPROC glBufferData;
    PFNGLMAPBUFFERPROC glMapBuffer;
    PFNGLUNMAPBUFFERPROC glUnmapBuffer;

    PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
    PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
    PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;

    PFNGLCREATESHADERPROC glCreateShader;
    PFNGLDELETESHADERPROC glDeleteShader;
    PFNGLSHADERSOURCEPROC glShaderSource;
    PFNGLCOMPILESHADERPROC glCompileShader;
    PFNGLGETSHADERIVPROC glGetShaderiv;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;

    PFNGLCREATEPROGRAMPROC glCreateProgram;
    PFNGLDELETEPROGRAMPROC glDeleteProgram;
    PFNGLUSEPROGRAMPROC glUseProgram;
    PFNGLATTACHSHADERPROC glAttachShader;
    PFNGLDETACHSHADERPROC glDetachShader;
    PFNGLLINKPROGRAMPROC glLinkProgram;
    PFNGLGETPROGRAMIVPROC glGetProgramiv;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
    PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
    PFNGLUNIFORM1IPROC glUniform1i;
    PFNGLUNIFORM3FVPROC glUniform3fv;
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix3fv;
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;

    static void* LoadProcOrDie(IGLContext& context, const char* procName);

    using ProgramPtr = std::unique_ptr<GLuint,std::function<void(GLuint*)>>;

    void CompileShader(GLuint handle, const char* src);

    ProgramPtr CompileProgram(const char* vsrc, const char* fsrc);

    class Pass
    {
    public:
        std::vector<RenderObject>& RenderObjects;
        std::vector<RenderCamera>& RenderCameras;

        std::vector<GLenum> FlagsToEnable;
        std::vector<GLenum> FlagsToDisable;
    };

    void RenderPass(const Pass& pass);

    ProgramPtr mColoredProgram;
    ProgramPtr mNormalColoredProgram;
    ProgramPtr mTexturedProgram;
    ProgramPtr mVertexColoredProgram;

public:
    OpenGLES2CommandVisitor(
            IGLContext& context,
            IWindow& window);

    void Visit(BeginFrameCommand& cmd) override;

    void Visit(EndFrameCommand& cmd) override;

    void Visit(RenderBatchCommand& cmd) override;

    void Visit(QuitCommand& cmd) override;

    bool ShouldQuit() override;
};

} // end namespace ng

#endif // NG_OPENGLES2COMMANDVISITOR_HPP
