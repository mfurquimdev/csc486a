#ifndef NG_GLRENDERER_HPP
#define NG_GLRENDERER_HPP

#include "ng/engine/renderer.hpp"

#include "ng/engine/resource.hpp"

#include <GL/gl.h>

#include <memory>
#include <thread>
#include <mutex>

namespace ng
{

class IWindowManager;
class IWindow;
class IGLContext;
struct RenderingOpenGLThreadData;
struct ResourceOpenGLThreadData;
struct OpenGLInstruction;

class OpenGLRenderer: public IRenderer, public std::enable_shared_from_this<OpenGLRenderer>
{
public:
    enum OpenGLInstructionHandler
    {
        RenderingInstructionHandler,
        ResourceInstructionHandler
    };

    // classes of objects created by Renderer
    static constexpr ResourceHandle::ClassID GLSharedFutureToBufferClassID{ "GLBF" };

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

    // for generating unique IDs for resources
    ResourceHandle::InstanceID mCurrentID = 0;
    std::mutex mIDGenLock;

    ResourceHandle::InstanceID GenerateInstanceID();

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

    ResourceHandle SendGenBuffer();

    void SendDeleteBuffer(GLuint buffer);

    void SendBufferData(
            OpenGLInstructionHandler instructionHandler,
            ResourceHandle bufferHandle,
            GLenum target,
            GLsizeiptr size,
            std::shared_ptr<const void> dataHandle,
            GLenum usage);

    void Clear(bool color, bool depth, bool stencil) override;

    void SwapBuffers() override;

    std::shared_ptr<IStaticMesh> CreateStaticMesh() override;
};

} // end namespace ng

#endif // NG_GLRENDERER_HPP
