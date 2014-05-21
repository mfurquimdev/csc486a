#ifndef NG_GLRENDERER_HPP
#define NG_GLRENDERER_HPP

#include "ng/engine/renderer.hpp"

#include <GL/gl.h>

#include <memory>
#include <thread>
#include <mutex>
#include <future>

namespace ng
{

class IWindowManager;
class IWindow;
class IGLContext;
class RenderingOpenGLThreadData;
class ResourceOpenGLThreadData;
struct OpenGLInstruction;
class OpenGLBufferHandle;
class VertexArray;
class OpenGLShaderHandle;
class OpenGLShaderProgramHandle;

class OpenGLRenderer: public IRenderer, public std::enable_shared_from_this<OpenGLRenderer>
{
public:
    enum OpenGLInstructionHandler
    {
        RenderingInstructionHandler,
        ResourceInstructionHandler
    };

private:
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mRenderingContext;
    std::shared_ptr<IGLContext> mResourceContext;

    std::unique_ptr<RenderingOpenGLThreadData> mRenderingThreadData;
    std::unique_ptr<ResourceOpenGLThreadData> mResourceThreadData;

    std::thread mRenderingThread;
    std::thread mResourceThread;

    static const std::size_t OneMB = 0x100000;
    static const std::size_t RenderingCommandBufferSize = OneMB;
    static const std::size_t ResourceCommandBufferSize = OneMB;

    void PushRenderingInstruction(const OpenGLInstruction& inst);

    void PushResourceInstruction(const OpenGLInstruction& inst);

    void PushInstruction(OpenGLInstructionHandler instructionHandler, const OpenGLInstruction& inst);

    void SwapRenderingInstructionQueues();

public:
    OpenGLRenderer(
            std::shared_ptr<IWindowManager> windowManager,
            std::shared_ptr<IWindow> window);

    ~OpenGLRenderer();

    void SendQuit(OpenGLInstructionHandler instructionHandler);

    void SendSwapBuffers();

    std::shared_future<OpenGLBufferHandle> SendGenBuffer();

    void SendDeleteBuffer(GLuint buffer);

    void SendBufferData(
            OpenGLInstructionHandler instructionHandler,
            std::shared_future<OpenGLBufferHandle> bufferHandle,
            GLenum target,
            GLsizeiptr size,
            std::shared_ptr<const void> dataHandle,
            GLenum usage);

    std::shared_future<OpenGLShaderHandle> SendGenShader();

    void SendDeleteShader(GLuint shader);

    void SendCompileShader(
            std::shared_future<OpenGLShaderHandle> shaderHandle,
            std::shared_ptr<const char> shaderSource);

    std::shared_future<std::pair<bool,std::string>> SendGetShaderStatus(std::shared_future<OpenGLShaderHandle> shader);

    std::shared_future<OpenGLShaderProgramHandle> SendGenShaderProgram();

    void SendDeleteShaderProgram(GLuint program);

    void SendLinkProgram(
            std::shared_future<OpenGLShaderProgramHandle> programHandle,
            std::shared_future<OpenGLShaderHandle> vertexShaderHandle,
            std::shared_future<OpenGLShaderHandle> fragmentShaderHandle);

    std::shared_future<std::pair<bool,std::string>> SendGetProgramStatus(std::shared_future<OpenGLShaderProgramHandle> program);

    void SendDrawVertexArray(
            const VertexArray& vertexArray,
            std::shared_future<OpenGLShaderProgramHandle> program,
            GLenum mode,
            GLint firstVertexIndex,
            GLsizei vertexCount);

    void Clear(bool color, bool depth, bool stencil) override;

    void SwapBuffers() override;

    std::shared_ptr<IStaticMesh> CreateStaticMesh() override;

    std::shared_ptr<IShaderProgram> CreateShaderProgram() override;
};

} // end namespace ng

#endif // NG_GLRENDERER_HPP
