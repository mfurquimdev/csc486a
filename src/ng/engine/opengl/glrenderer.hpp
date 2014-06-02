#ifndef NG_GLRENDERER_HPP
#define NG_GLRENDERER_HPP

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/opengl/globject.hpp"

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
class VertexArray;

enum class RenderingMode
{
    Synchronous,
    Asynchronous
};

class OpenGLRenderer: public IRenderer, public std::enable_shared_from_this<OpenGLRenderer>
{
public:
    enum OpenGLInstructionHandler
    {
        RenderingInstructionHandler,
        ResourceInstructionHandler
    };

private:
    RenderingMode mRenderingMode;

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
            std::shared_ptr<IWindow> window,
            RenderingMode renderingMode);

    ~OpenGLRenderer();

    void SendQuit(OpenGLInstructionHandler instructionHandler);

    void SendSwapBuffers();

    OpenGLFuture<std::shared_ptr<OpenGLBufferHandle>> SendGenBuffer();

    void SendDeleteBuffer(GLuint buffer);

    OpenGLFuture<std::shared_ptr<OpenGLBufferHandle>> SendBufferData(
            OpenGLInstructionHandler instructionHandler,
            OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>> bufferHandle,
            GLenum target,
            GLsizeiptr size,
            std::shared_ptr<const void> dataHandle,
            GLenum usage);

    OpenGLFuture<std::shared_ptr<OpenGLVertexArrayHandle>> SendGenVertexArray();

    void SendDeleteVertexArray(GLuint vertexArray);

    OpenGLFuture<std::shared_ptr<OpenGLVertexArrayHandle>> SendSetVertexArrayLayout(
            OpenGLSharedFuture<std::shared_ptr<OpenGLVertexArrayHandle>> vertexArrayHandle,
            VertexFormat format,
            std::map<VertexAttributeName,OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>>> attributeBuffers,
            OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>> indexBuffer);

    OpenGLFuture<std::shared_ptr<OpenGLShaderHandle>> SendGenShader(GLenum shaderType);

    void SendDeleteShader(GLuint shader);

    OpenGLFuture<std::shared_ptr<OpenGLShaderHandle>> SendCompileShader(
            OpenGLSharedFuture<std::shared_ptr<OpenGLShaderHandle>> shaderHandle,
            std::shared_ptr<const char> shaderSource);

    OpenGLFuture<std::pair<bool,std::string>> SendGetShaderStatus(OpenGLSharedFuture<std::shared_ptr<OpenGLShaderHandle>> shader);

    OpenGLFuture<std::shared_ptr<OpenGLShaderProgramHandle>> SendGenShaderProgram();

    void SendDeleteShaderProgram(GLuint program);

    OpenGLFuture<std::shared_ptr<OpenGLShaderProgramHandle>> SendLinkProgram(
            OpenGLSharedFuture<std::shared_ptr<OpenGLShaderProgramHandle>> programHandle,
            OpenGLSharedFuture<std::shared_ptr<OpenGLShaderHandle>> vertexShaderHandle,
            OpenGLSharedFuture<std::shared_ptr<OpenGLShaderHandle>> fragmentShaderHandle);

    OpenGLFuture<std::pair<bool,std::string>> SendGetProgramStatus(OpenGLSharedFuture<std::shared_ptr<OpenGLShaderProgramHandle>> program);

    void SendDrawVertexArray(
            OpenGLSharedFuture<std::shared_ptr<OpenGLVertexArrayHandle>> vertexArray,
            OpenGLSharedFuture<std::shared_ptr<OpenGLShaderProgramHandle>> program,
            std::map<std::string, UniformValue> uniforms,
            RenderState renderState,
            GLenum mode,
            GLint firstVertexIndex,
            GLsizei vertexCount,
            bool isIndexed,
            ArithmeticType indexType);

    void Clear(bool color, bool depth, bool stencil) override;

    void SwapBuffers() override;

    std::shared_ptr<IStaticMesh> CreateStaticMesh() override;

    std::shared_ptr<IShaderProgram> CreateShaderProgram() override;
};

} // end namespace ng

#endif // NG_GLRENDERER_HPP
