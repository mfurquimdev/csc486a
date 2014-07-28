#include "ng/engine/opengl/opengles2commandvisitor.hpp"

#include "ng/engine/window/window.hpp"
#include "ng/engine/window/glcontext.hpp"

#include "ng/engine/rendering/mesh.hpp"
#include "ng/engine/rendering/material.hpp"
#include "ng/engine/rendering/texture.hpp"

#include "ng/engine/opengl/openglenumconversion.hpp"

#include "ng/engine/util/scopeguard.hpp"
#include "ng/engine/util/debug.hpp"

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
        const char* vsrc, const char* fsrc)
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

    program.get_deleter() = [vshader, fshader, this](GLuint* prog){
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
    GetGLExtensionOrDie(context, glUniform1i);
    GetGLExtensionOrDie(context, glUniform3fv);
    GetGLExtensionOrDie(context, glUniformMatrix3fv);
    GetGLExtensionOrDie(context, glUniformMatrix4fv);

    static const char* coloredVSrc =
            "#version 100\n"

            "uniform highp mat4 uProjection;\n"
            "uniform highp mat4 uModelView;\n"

            "attribute highp vec4 iPosition;\n"

            "void main() {\n"
            "    gl_Position = uProjection * uModelView * iPosition;\n"
            "}\n";

    static const char* coloredFSrc =
            "#version 100\n"

            "uniform highp vec3 uTint;\n"

            "void main() {\n"
             "    gl_FragColor = vec4(uTint,1.0);\n"
            "}\n";

    mColoredProgram = CompileProgram(
                coloredVSrc, coloredFSrc);

    static const char* normalColoredVSrc =
            "#version 100\n"

            "uniform highp mat4 uProjection;\n"
            "uniform highp mat4 uModelView;\n"
            "uniform highp mat3 uModelWorldNormalMatrix;\n"

            "attribute highp vec4 iPosition;\n"
            "attribute highp vec3 iNormal;\n"

            "varying highp vec3 fViewNormal;\n"

            "void main() {\n"
            "    gl_Position = uProjection * uModelView * iPosition;\n"
            "    fViewNormal = uModelWorldNormalMatrix * iNormal;\n"
            "}\n";

    static const char* normalColoredFSrc =
            "#version 100\n"

            "varying highp vec3 fViewNormal;\n"

            "void main() {\n"
            "    gl_FragColor = vec4((fViewNormal + vec3(1)) / vec3(2), 1.0);\n"
            "}\n";

    mNormalColoredProgram = CompileProgram(
                normalColoredVSrc, normalColoredFSrc);

    static const char* texturedVSrc =
            "#version 100\n"

            "uniform highp mat4 uProjection;\n"
            "uniform highp mat4 uModelView;\n"

            "attribute highp vec4 iPosition;\n"
            "attribute highp vec2 iTexcoord0;\n"

            "varying highp vec2 fTexcoord0;\n"

            "void main() {\n"
            "    gl_Position = uProjection * uModelView * iPosition;\n"
            "    fTexcoord0 = iTexcoord0;\n"
            "}\n";

    static const char* texturedFSrc =
            "#version 100\n"

            "uniform sampler2D uTexture0;\n"

            "varying highp vec2 fTexcoord0;\n"

            "void main() {\n"
            "    gl_FragColor = texture2D(uTexture0, fTexcoord0);\n"
            "}\n";

    mTexturedProgram = CompileProgram(
                texturedVSrc, texturedFSrc);

    static const char* vertexColoredVSrc =
            "#version 100\n"

            "uniform highp mat4 uProjection;\n"
            "uniform highp mat4 uModelView;\n"

            "attribute highp vec4 iPosition;\n"
            "attribute highp vec4 iColor;\n"

            "varying highp vec4 fColor;\n"

            "void main() {\n"
            "    gl_Position = uProjection * uModelView * iPosition;\n"
            "    fColor = iColor;\n"
            "}\n";

    static const char* vertexColoredFSrc =
            "#version 100\n"

            "varying highp vec4 fColor;\n"

            "void main() {\n"
            "    gl_FragColor = fColor;\n"
            "}\n";

    mVertexColoredProgram = CompileProgram(
                vertexColoredVSrc, vertexColoredFSrc);
}

#undef GetGLExtension

void OpenGLES2CommandVisitor::Visit(BeginFrameCommand& cmd)
{
    vec3 clear = cmd.ClearColor;
    glClearColor(clear.x, clear.y, clear.z, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT  |
            GL_DEPTH_BUFFER_BIT  |
            GL_STENCIL_BUFFER_BIT);
}

void OpenGLES2CommandVisitor::Visit(EndFrameCommand&)
{
    mWindow->SwapBuffers();
}

void OpenGLES2CommandVisitor::RenderPass(const Pass& pass)
{
    for (GLenum flag : pass.FlagsToEnable)
    {
        glEnable(flag);
    }

    for (GLenum flag : pass.FlagsToDisable)
    {
        glDisable(flag);
    }

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

    GLuint texture0;
    glGenTextures(1, &texture0);
    auto texture0Scope = make_scope_guard([&]{
        glDeleteTextures(1, &texture0);
    });

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBindTexture(GL_TEXTURE_2D, texture0);

    for (const RenderCamera& cam : pass.RenderCameras)
    {
        mat4 worldView = cam.WorldView;
        mat4 projection = cam.Projection;

        glViewport(
                cam.ViewportTopLeft.x, cam.ViewportTopLeft.y,
                cam.ViewportSize.x, cam.ViewportSize.y);

        for (const RenderObject& obj : pass.RenderObjects)
        {
            if (obj.Mesh == nullptr || obj.Material.Type == MaterialType::Null)
            {
                continue;
            }

            const IMesh& mesh = *obj.Mesh;
            const Material& mat = obj.Material;

            GLuint program = 0;
            if (mat.Type == MaterialType::Colored ||
                mat.Type == MaterialType::Wireframe)
            {
                program = *mColoredProgram;
            }
            else if (mat.Type == MaterialType::NormalColored)
            {
                program = *mNormalColoredProgram;
            }
            else if (mat.Type == MaterialType::Textured)
            {
                program = *mTexturedProgram;
            }
            else if (mat.Type == MaterialType::VertexColored)
            {
                program = *mVertexColoredProgram;
            }
            else
            {
                throw std::logic_error("Unhandled material type");
            }

            glUseProgram(program);

            mat4 modelView = worldView * obj.WorldTransform;

            mat3 modelWorldNormalMatrix =
                mat3(transpose(inverse(obj.WorldTransform)));

            mat3 normalMatrix = mat3(transpose(inverse(modelView)));

            VertexFormat fmt = mesh.GetVertexFormat();

            std::size_t maxVBOSize = mesh.GetMaxVertexBufferSize();
            std::size_t numVertices = 0;

            std::size_t maxEBOSize = mesh.GetMaxIndexBufferSize();
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
                                GL_ELEMENT_ARRAY_BUFFER,
                                maxEBOSize,
                                elementBuffer.get(),
                                GL_STREAM_DRAW);
                }
            }

            GLint projectionLoc = glGetUniformLocation(program, "uProjection");

            if (projectionLoc != -1)
            {
                glUniformMatrix4fv(
                            projectionLoc,
                            1,
                            GL_FALSE,
                            &projection[0][0]);
            }

            GLint modelViewLoc = glGetUniformLocation(program, "uModelView");

            if (modelViewLoc != -1)
            {
                glUniformMatrix4fv(
                            modelViewLoc,
                            1,
                            GL_FALSE,
                            &modelView[0][0]);
            }

            GLint normalMatrixLoc =
                glGetUniformLocation(program, "uNormalMatrix");

            if (normalMatrixLoc != -1)
            {
                glUniformMatrix3fv(
                            normalMatrixLoc,
                            1,
                            GL_FALSE,
                            &normalMatrix[0][0]);
            }

            GLint modelWorldNormalMatrixLoc =
                glGetUniformLocation(program, "uModelWorldNormalMatrix");

            if (modelWorldNormalMatrixLoc != -1)
            {
                glUniformMatrix3fv(
                            modelWorldNormalMatrixLoc,
                            1,
                            GL_FALSE,
                            &modelWorldNormalMatrix[0][0]);
            }

            GLint tintLoc = glGetUniformLocation(program, "uTint");

            if (tintLoc != -1)
            {
                glUniform3fv(tintLoc, 1, &mat.Tint[0]);
            }

            GLint texture0Loc = glGetUniformLocation(program, "uTexture0");

            if (texture0Loc != -1)
            {
                glUniform1i(texture0Loc, 0);

                const ITexture& tex = *mat.Texture0;

                TextureFormat fmt = tex.GetTextureFormat();

                if (fmt.Format == ImageFormat::Invalid)
                {
                    throw std::logic_error("Invalid texture ImageFormat");
                }

                if (fmt.Type == TextureType::Invalid)
                {
                    throw std::logic_error("Invalid TextureType");
                }

                const Sampler& sampler = mat.Sampler0;

                if (sampler.MinFilter == TextureFilter::Invalid ||
                    sampler.MagFilter == TextureFilter::Invalid)
                {
                    throw std::logic_error("Invalid TextureFilter");
                }

                if (sampler.WrapX == TextureWrap::Invalid ||
                    sampler.WrapY == TextureWrap::Invalid)
                {
                    throw std::logic_error("Invalid TextureWrap");
                }

#ifndef NG_USE_EMSCRIPTEN
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    ToGLTextureFilter(sampler.MinFilter));

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    ToGLTextureFilter(sampler.MagFilter));

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    ToGLTextureWrap(sampler.WrapX));

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    ToGLTextureWrap(sampler.WrapY));

                std::unique_ptr<char[]> texData(
                    new char[fmt.Width * fmt.Height * fmt.Depth * 4]);

                tex.WriteTextureData(texData.get());

                glTexImage2D(GL_TEXTURE_2D, 0,
                    GL_RGBA, fmt.Width, fmt.Height,
                    0,
                    GL_RGBA, GL_UNSIGNED_BYTE, texData.get());
            }

            if (fmt.Position.Enabled)
            {
                const VertexAttribute& posAttr = fmt.Position;

                GLint posLoc = glGetAttribLocation(program, "iPosition");

                if (posLoc != -1)
                {
                    glEnableVertexAttribArray(posLoc);

                    glVertexAttribPointer(
                                posLoc,
                                posAttr.Cardinality,
                                ToGLArithmeticType(posAttr.Type),
                                posAttr.Normalized,
                                posAttr.Stride,
                                reinterpret_cast<const GLvoid*>(
                                    posAttr.Offset));
                }
            }

            if (fmt.Normal.Enabled)
            {
                const VertexAttribute& nAttr = fmt.Normal;

                GLint nLoc = glGetAttribLocation(program, "iNormal");

                if (nLoc != -1)
                {
                    glEnableVertexAttribArray(nLoc);

                    glVertexAttribPointer(
                                nLoc,
                                nAttr.Cardinality,
                                ToGLArithmeticType(nAttr.Type),
                                nAttr.Normalized,
                                nAttr.Stride,
                                reinterpret_cast<const GLvoid*>(nAttr.Offset));
                }
            }

            if (fmt.TexCoord0.Enabled)
            {
                const VertexAttribute& tAttr = fmt.TexCoord0;

                GLint tLoc = glGetAttribLocation(program, "iTexcoord0");

                if (tLoc != -1)
                {
                    glEnableVertexAttribArray(tLoc);

                    glVertexAttribPointer(
                                tLoc,
                                tAttr.Cardinality,
                                ToGLArithmeticType(tAttr.Type),
                                tAttr.Normalized,
                                tAttr.Stride,
                                reinterpret_cast<const GLvoid*>(tAttr.Offset));
                }
            }

            if (fmt.Color.Enabled)
            {
                const VertexAttribute& cAttr = fmt.Color;

                GLint cLoc = glGetAttribLocation(program, "iColor");

                if (cLoc != -1)
                {
                    glEnableVertexAttribArray(cLoc);

                    glVertexAttribPointer(
                                cLoc,
                                cAttr.Cardinality,
                                ToGLArithmeticType(cAttr.Type),
                                cAttr.Normalized,
                                cAttr.Stride,
                                reinterpret_cast<const GLvoid*>(cAttr.Offset));
                }
            }

            GLenum primitiveType = ToGLPrimitive(fmt.PrimitiveType);

            if (numElements > 0)
            {
                if (mat.Type == ng::MaterialType::Wireframe &&
                    primitiveType == GL_TRIANGLES)
                {
                    // ouch
                    for (std::size_t i = 0; i < numElements; i += 3)
                    {
                        glDrawElements(
                            GL_LINE_LOOP,
                            3,
                            ToGLArithmeticType(fmt.IndexType),
                            reinterpret_cast<const GLvoid*>(
                                fmt.IndexOffset
                              + i * SizeOfArithmeticType(fmt.IndexType)));
                    }
                }
                else
                {
                    glDrawElements(
                                primitiveType,
                                numElements,
                                ToGLArithmeticType(fmt.IndexType),
                                reinterpret_cast<const GLvoid*>(fmt.IndexOffset));
                }
            }
            else if (numVertices > 0)
            {
                if (mat.Type == ng::MaterialType::Wireframe &&
                    primitiveType == GL_TRIANGLES)
                {
                    // ouch
                    for (std::size_t i = 0; i < numVertices; i += 3)
                    {
                        glDrawArrays(GL_LINE_LOOP, i, 3);
                    }
                }
                else
                {
                    glDrawArrays(
                                primitiveType,
                                0,
                                numVertices);
                }
            }
        }
    }
}

void OpenGLES2CommandVisitor::Visit(RenderBatchCommand& cmd)
{
    Pass scenePass{
        cmd.Batch.RenderObjects,
        cmd.Batch.RenderCameras,
        { GL_DEPTH_TEST },
        { }
    };

    Pass overlayPass{
        cmd.Batch.OverlayRenderObjects,
        cmd.Batch.OverlayRenderCameras,
        { },
        { GL_DEPTH_TEST }
    };

    // sort overlay back-to-front, assuming everything is flat.
    std::sort(overlayPass.RenderObjects.begin(),
              overlayPass.RenderObjects.end(),
              [](const RenderObject& a, const RenderObject& b)
    {
        return (a.WorldTransform * vec4(0,0,0,1)).z <
               (b.WorldTransform * vec4(0,0,0,1)).z;
    });

    RenderPass(scenePass);
    RenderPass(overlayPass);
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
