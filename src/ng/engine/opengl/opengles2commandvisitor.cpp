#include "ng/engine/opengl/opengles2commandvisitor.hpp"

#include "ng/engine/window/window.hpp"
#include "ng/engine/window/glcontext.hpp"

#include "ng/engine/rendering/mesh.hpp"

#include "ng/engine/opengl/openglenumconversion.hpp"

#include "ng/engine/util/scopeguard.hpp"

#include <string>

namespace ng
{

void* OpenGLES2CommandVisitor::LoadProcOrDie(
        IGLContext& context,
        const char* procName)
{
    auto ext = context.GetProcAddress(procName);
    if (!ext)
    {
        throw std::runtime_error(
                    std::string("Failed to load GL extension: ") + procName);
    }
    return ext;
}

void OpenGLES2CommandVisitor::CompileShader(GLuint handle, const char* src)
{
    glShaderSource(handle, 1, &src, NULL);
    glCompileShader(handle);

    GLint status;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint logLength;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0)
        {
            std::unique_ptr<char[]> log(new char[logLength]);
            glGetShaderInfoLog(handle, logLength, NULL, log.get());

            throw std::runtime_error(
                        std::string("Failed to compile shader:\n") +
                        log.get());
        }
        else
        {
            throw std::runtime_error("Failed to compile shader: (no info)");
        }
    }
}

OpenGLES2CommandVisitor::ProgramPtr OpenGLES2CommandVisitor::CompileProgram(
        const char *vsrc, const char *fsrc)
{
    ProgramPtr program;

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    auto vshaderScope = make_scope_guard([&]{
       glDeleteShader(vshader);
    });

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    auto fshaderScope = make_scope_guard([&]{
       glDeleteShader(fshader);
    });

    program.get_deleter() = [&](GLuint* prog){
        if (prog && *prog)
        {
            glDetachShader(*prog, vshader);
            glDetachShader(*prog, fshader);
            glDeleteProgram(*prog);
        }
        delete prog;
    };

    program.reset(new GLuint(0));
    *program = glCreateProgram();

    glAttachShader(*program, vshader);
    glAttachShader(*program, fshader);

    CompileShader(vshader, vsrc);
    CompileShader(fshader, fsrc);

    glLinkProgram(*program);

    GLint status;
    glGetProgramiv(*program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint logLength;
        glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0)
        {
            std::unique_ptr<char[]> log(new char[logLength]);
            glGetProgramInfoLog(*program, logLength, NULL, log.get());

            throw std::runtime_error(
                        std::string("Failed to link program:\n") +
                        log.get());
        }
        else
        {
            throw std::runtime_error("Failed to link program: (no info)");
        }
    }

    return std::move(program);
}

// used by GetGLExtension
#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif

// loads a single extension
#define GetGLExtensionOrDie(context, ExtensionFunctionName) \
    ExtensionFunctionName = \
        reinterpret_cast<decltype(ExtensionFunctionName)>( \
            LoadProcOrDie(context, STRINGIFY(ExtensionFunctionName)))

#define TryGetGLExtension(context, ExtensionFunctionName) \
    ExtensionFunctionName = \
        reinterpret_cast<decltype(ExtensionFunctionName)>( \
            context.GetProcAddress(STRINGIFY(ExtensionFunctionName)))

OpenGLES2CommandVisitor::OpenGLES2CommandVisitor(
        IGLContext& context,
        IWindow& window)
    : mGLContext(&context)
    , mWindow(&window)
{
    GetGLExtensionOrDie(context, glGenBuffers);
    GetGLExtensionOrDie(context, glDeleteBuffers);
    GetGLExtensionOrDie(context, glBindBuffer);
    GetGLExtensionOrDie(context, glBufferData);
    TryGetGLExtension(context, glMapBuffer);
    TryGetGLExtension(context, glUnmapBuffer);

    GetGLExtensionOrDie(context, glGenVertexArrays);
    GetGLExtensionOrDie(context, glDeleteVertexArrays);
    GetGLExtensionOrDie(context, glBindVertexArray);
    GetGLExtensionOrDie(context, glVertexAttribPointer);
    GetGLExtensionOrDie(context, glEnableVertexAttribArray);
    GetGLExtensionOrDie(context, glDisableVertexAttribArray);

    GetGLExtensionOrDie(context, glCreateShader);
    GetGLExtensionOrDie(context, glDeleteShader);
    GetGLExtensionOrDie(context, glShaderSource);
    GetGLExtensionOrDie(context, glCompileShader);
    GetGLExtensionOrDie(context, glGetShaderiv);
    GetGLExtensionOrDie(context, glGetShaderInfoLog);

    GetGLExtensionOrDie(context, glCreateProgram);
    GetGLExtensionOrDie(context, glDeleteProgram);
    GetGLExtensionOrDie(context, glUseProgram);
    GetGLExtensionOrDie(context, glAttachShader);
    GetGLExtensionOrDie(context, glDetachShader);
    GetGLExtensionOrDie(context, glLinkProgram);
    GetGLExtensionOrDie(context, glGetProgramiv);
    GetGLExtensionOrDie(context, glGetProgramInfoLog);
    GetGLExtensionOrDie(context, glGetAttribLocation);
    GetGLExtensionOrDie(context, glGetUniformLocation);
    GetGLExtensionOrDie(context, glUniformMatrix4fv);

    static const char* debug_vsrc =
            "#version 100\n"

            "uniform highp mat4 uProjection;\n"
            "uniform highp mat4 uModelView;\n"

            "attribute highp vec4 iPosition;\n"
            "varying highp vec4 fPosition;\n"

            "void main() {\n"
            " gl_Position = uProjection * uModelView * iPosition;\n"
            " fPosition = iPosition;\n"
            "}\n";

    static const char* debug_fsrc =
            "#version 100\n"

            "varying highp vec4 fPosition;\n"

            "void main() {\n"
            " gl_FragColor = vec4(fPosition.xyz,1.0);\n"
            "}\n";

    mDebugProgram = CompileProgram(debug_vsrc, debug_fsrc);
}

#undef GetGLExtension

void OpenGLES2CommandVisitor::Visit(BeginFrameCommand&)
{
    glClear(GL_COLOR_BUFFER_BIT  |
            GL_DEPTH_BUFFER_BIT  |
            GL_STENCIL_BUFFER_BIT);
}

void OpenGLES2CommandVisitor::Visit(EndFrameCommand&)
{
    mWindow->SwapBuffers();
}

void OpenGLES2CommandVisitor::Visit(RenderBatchCommand& cmd)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    auto vertexBufferScope = make_scope_guard([&]{
        glDeleteBuffers(1, &vbo);
    });

    GLuint ebo;
    glGenBuffers(1, &ebo);
    auto elementBufferScope = make_scope_guard([&]{
        glDeleteBuffers(1, &ebo);
    });

    GLuint vao;
    glGenVertexArrays(1, &vao);
    auto vertexArrayScope = make_scope_guard([&]{
       glDeleteVertexArrays(1, &vao);
    });

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    for (const RenderCamera& cam : cmd.Batch.RenderCameras)
    {
        mat4 worldView = cam.WorldView;
        mat4 projection = cam.Projection;

        glViewport(
                cam.ViewportTopLeft.x, cam.ViewportTopLeft.y,
                cam.ViewportSize.x, cam.ViewportSize.y);

        for (const RenderObject& obj : cmd.Batch.RenderObjects)
        {
            if (obj.Mesh == nullptr)
            {
                continue;
            }

            const IMesh& mesh = *obj.Mesh;

            GLuint program = *mDebugProgram;

            glUseProgram(program);

            mat4 modelView = worldView * obj.WorldTransform;

            VertexFormat fmt = mesh.GetVertexFormat();

            std::size_t maxVBOSize = mesh.GetMaxVertexBufferSize();
            std::size_t numVertices = 0;

            std::size_t maxEBOSize = mesh.GetMaxElementBufferSize();
            std::size_t numElements = 0;

            if (maxVBOSize > 0)
            {
                if (glMapBuffer != nullptr)
                {
                    glBufferData(
                                GL_ARRAY_BUFFER,
                                maxVBOSize,
                                NULL,
                                GL_STREAM_DRAW);

                    void* vertexBuffer =
                            glMapBuffer(
                                GL_ARRAY_BUFFER, GL_WRITE_ONLY);

                    auto mapScope = make_scope_guard([&]{
                        glUnmapBuffer(GL_ARRAY_BUFFER);
                    });

                    numVertices = mesh.WriteVertices(vertexBuffer);
                }
                else
                {
                    std::unique_ptr<char[]> vertexBuffer(new char[maxVBOSize]);
                    numVertices = mesh.WriteVertices(vertexBuffer.get());
                    glBufferData(
                                GL_ARRAY_BUFFER,
                                maxVBOSize,
                                vertexBuffer.get(),
                                GL_STREAM_DRAW);
                }
            }

            if (maxEBOSize > 0)
            {
                if (glMapBuffer != nullptr)
                {
                    glBufferData(
                                GL_ELEMENT_ARRAY_BUFFER,
                                maxEBOSize,
                                NULL,
                                GL_STREAM_DRAW);

                    void* elementBuffer =
                            glMapBuffer(
                                GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

                    auto mapScope = make_scope_guard([&]{
                        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
                    });

                    numElements = mesh.WriteIndices(elementBuffer);
                }
                else
                {
                    std::unique_ptr<char[]> elementBuffer(new char[maxEBOSize]);
                    numElements = mesh.WriteIndices(elementBuffer.get());
                    glBufferData(
                                GL_ARRAY_BUFFER,
                                maxVBOSize,
                                elementBuffer.get(),
                                GL_STREAM_DRAW);
                }
            }

            GLuint projectionLoc = glGetUniformLocation(program, "uProjection");

            glUniformMatrix4fv(
                        projectionLoc,
                        1,
                        GL_FALSE,
                        &projection[0][0]);

            GLuint modelViewLoc = glGetUniformLocation(program, "uModelView");

            glUniformMatrix4fv(
                        modelViewLoc,
                        1,
                        GL_FALSE,
                        &modelView[0][0]);

            if (fmt.Position.Enabled)
            {
                const VertexAttribute& posAttr = fmt.Position;

                GLint posLoc = glGetAttribLocation(program, "iPosition");

                glEnableVertexAttribArray(posLoc);

                glVertexAttribPointer(
                            posLoc,
                            posAttr.Cardinality,
                            ToGLArithmeticType(posAttr.Type),
                            posAttr.Normalized,
                            posAttr.Stride,
                            reinterpret_cast<const GLvoid*>(posAttr.Offset));
            }

            if (numElements > 0)
            {
                glDrawElements(
                            GL_TRIANGLES,
                            numElements,
                            ToGLArithmeticType(fmt.IndexType),
                            reinterpret_cast<const GLvoid*>(fmt.IndexOffset));
            }
            else if (numVertices > 0)
            {
                glDrawArrays(
                            GL_TRIANGLES,
                            0,
                            numVertices);
            }
        }
    }
}

void OpenGLES2CommandVisitor::Visit(QuitCommand&)
{
    mShouldQuit = true;
}

bool OpenGLES2CommandVisitor::ShouldQuit()
{
    return mShouldQuit;
}

} // end namespace ng
