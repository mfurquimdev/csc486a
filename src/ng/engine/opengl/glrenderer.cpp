#include "ng/engine/opengl/glrenderer.hpp"

#include "ng/engine/windowmanager.hpp"
#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"

#include "ng/engine/opengl/glinstruction.hpp"
#include "ng/engine/opengl/globject.hpp"
#include "ng/engine/opengl/glenumconversion.hpp"

#include "ng/engine/profiler.hpp"
#include "ng/engine/debug.hpp"
#include "ng/engine/semaphore.hpp"
#include "ng/engine/memory.hpp"

#include <cstring>
#include <utility>
#include <string>

#if 1
#define RenderDebugPrintf(...) DebugPrintf(__VA_ARGS__)
#else
#define RenderDebugPrintf(...)
#endif

#if 1
#define RenderProfilePrintf(...) DebugPrintf(__VA_ARGS__)
#else
#define RenderProfilePrintf(...)
#endif

namespace ng
{

class CommonOpenGLThreadData
{
public:
    CommonOpenGLThreadData(
            const std::string& threadName,
            const std::shared_ptr<IWindowManager>& windowManager,
            const std::shared_ptr<IWindow>& window,
            const std::shared_ptr<IGLContext>& context,
            OpenGLRenderer& renderer)
        : mThreadName(threadName)
        , mWindowManager(windowManager)
        , mWindow(window)
        , mContext(context)
        , mRenderer(renderer)
    { }

    std::string mThreadName;

    std::shared_ptr<IWindowManager> mWindowManager;
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mContext;

    OpenGLRenderer& mRenderer;
};

class RenderingOpenGLThreadData : public CommonOpenGLThreadData
{
public:
    static const size_t InitialWriteBufferIndex = 0;

    RenderingOpenGLThreadData(
            const std::string& threadName,
            const std::shared_ptr<IWindowManager>& windowManager,
            const std::shared_ptr<IWindow>& window,
            const std::shared_ptr<IGLContext>& context,
            OpenGLRenderer& renderer,
            size_t instructionBufferSize)
        : CommonOpenGLThreadData(
              threadName,
              windowManager,
              window,
              context,
              renderer)
        , mInstructionBuffers{ instructionBufferSize, instructionBufferSize }
        , mCurrentWriteBufferIndex(InitialWriteBufferIndex)
    {
        // consumer mutexes are initially locked, since nothing has been produced yet.
        mInstructionConsumerMutex[0].lock();
        mInstructionConsumerMutex[1].lock();

        // the initial write buffer's producer mutex is locked since it's initially producing
        mInstructionProducerMutex[InitialWriteBufferIndex].lock();
    }

    // rendering is double buffered, so this is handled by producer/consumer
    // which is synchronized at every call to swap buffers at the end of a frame.
    OpenGLInstructionLinearBuffer mInstructionBuffers[2];
    std::mutex mInstructionProducerMutex[2];
    std::mutex mInstructionConsumerMutex[2];

    size_t mCurrentWriteBufferIndex;
    std::recursive_mutex mCurrentWriteBufferMutex;
};

class ResourceOpenGLThreadData : public CommonOpenGLThreadData
{
public:
    ResourceOpenGLThreadData(
            const std::string& threadName,
            const std::shared_ptr<IWindowManager>& windowManager,
            const std::shared_ptr<IWindow>& window,
            const std::shared_ptr<IGLContext>& context,
            OpenGLRenderer& renderer,
            size_t instructionBufferSize)
        : CommonOpenGLThreadData(
              threadName,
              windowManager,
              window,
              context,
              renderer)
        , mInstructionBuffer(instructionBufferSize)
    { }

    // the resource loading thread just loads resources as soon as it can all the time.
    // it knows to keep loading resources as long as the semaphore is up.
    ng::semaphore mConsumerSemaphore;

    OpenGLInstructionRingBuffer mInstructionBuffer;
    std::recursive_mutex mInstructionBufferMutex;
};

void OpenGLRenderer::PushRenderingInstruction(const OpenGLInstruction& inst)
{
    std::lock_guard<std::recursive_mutex> lock(mRenderingThreadData->mCurrentWriteBufferMutex);
    auto writeIndex = mRenderingThreadData->mCurrentWriteBufferIndex;

    if (!mRenderingThreadData->mInstructionBuffers[writeIndex].PushInstruction(inst))
    {
        // TODO: Need to flush the queue and try again? or maybe just report an error and give up?
        // currently the queue is automatically resized which will die fast in an infinite loop.
        throw std::overflow_error("Rendering instruction buffer too small for instructions. Increase size or improve OpenGLInstructionLinearBuffer");
    }
}

void OpenGLRenderer::PushResourceInstruction(const OpenGLInstruction& inst)
{
    std::lock_guard<std::recursive_mutex> lock(mResourceThreadData->mInstructionBufferMutex);

    if (!mResourceThreadData->mInstructionBuffer.PushInstruction(inst))
    {
        // TODO: Need to flush the queue and try again? or maybe just report an error and give up?
        // currently the queue is automatically resized which will die fast in an infinite loop.
        throw std::overflow_error("Resource instruction buffer too small for instructions. Increase size or improve OpenGLInstructionRingBuffer");
    }

    mResourceThreadData->mConsumerSemaphore.post();
}

void OpenGLRenderer::PushInstruction(OpenGLInstructionHandler instructionHandler, const OpenGLInstruction& inst)
{
    if (instructionHandler == RenderingInstructionHandler)
    {
        PushRenderingInstruction(inst);
    }
    else if (instructionHandler == ResourceInstructionHandler)
    {
        PushResourceInstruction(inst);
    }
    else
    {
        throw std::logic_error("Unkonwn OpenGL instruction handler");
    }
}

void OpenGLRenderer::SwapRenderingInstructionQueues()
{
    // make sure nobody else is relying on the current write buffer to stay the same.
    std::lock_guard<std::recursive_mutex> indexLock(mRenderingThreadData->mCurrentWriteBufferMutex);
    auto finishedWriteIndex = mRenderingThreadData->mCurrentWriteBufferIndex;

    // must have production rights to be able to start writing to the other thread.
    mRenderingThreadData->mInstructionProducerMutex[!finishedWriteIndex].lock();

    // start writing to the other thread.
    mRenderingThreadData->mCurrentWriteBufferIndex = !mRenderingThreadData->mCurrentWriteBufferIndex;

    // allow consumer to begin reading what was just written
    mRenderingThreadData->mInstructionConsumerMutex[finishedWriteIndex].unlock();
}

static void OpenGLRenderingThreadEntry(RenderingOpenGLThreadData* threadData);
static void OpenGLResourceThreadEntry(ResourceOpenGLThreadData* threadData);

OpenGLRenderer::OpenGLRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window)
    : mWindow(std::move(window))
    , mRenderingContext(windowManager->CreateContext(mWindow->GetVideoFlags(), nullptr))
    , mResourceContext(windowManager->CreateContext(mWindow->GetVideoFlags(), mRenderingContext))
{
    mRenderingThreadData.reset(new RenderingOpenGLThreadData(
                                   "OpenGL_Rendering",
                                   windowManager,
                                   mWindow,
                                   mRenderingContext,
                                   *this,
                                   RenderingCommandBufferSize));

    mResourceThreadData.reset(new ResourceOpenGLThreadData(
                                  "OpenGL_Resources",
                                  windowManager,
                                  mWindow,
                                  mResourceContext,
                                  *this,
                                  ResourceCommandBufferSize));

    mRenderingThread = std::thread(OpenGLRenderingThreadEntry, mRenderingThreadData.get());
    mResourceThread = std::thread(OpenGLResourceThreadEntry, mResourceThreadData.get());
}

OpenGLRenderer::~OpenGLRenderer()
{
    // add a Quit instruction to the resource instructions then leave it to be executed.
    SendQuit(ResourceInstructionHandler);

    // add a Quit instruction to the current buffer then swap away from it and leave it to be executed.
    std::unique_lock<std::recursive_mutex> renderingWriteLock(mRenderingThreadData->mCurrentWriteBufferMutex);
    auto renderingWrittenBufferIndex = mRenderingThreadData->mCurrentWriteBufferIndex;
    SendQuit(RenderingInstructionHandler);
    SwapRenderingInstructionQueues();
    mRenderingThreadData->mInstructionConsumerMutex[renderingWrittenBufferIndex].unlock();
    renderingWriteLock.unlock();

    // finally join everything, which waits for both their Quit commands to be run.
    mResourceThread.join();
    mRenderingThread.join();
}

void OpenGLRenderer::SendQuit(OpenGLInstructionHandler instructionHandler)
{
    auto si = QuitOpCodeParams().ToInstruction();
    PushInstruction(instructionHandler, si.Instruction);
}

void OpenGLRenderer::SendSwapBuffers()
{
    PushRenderingInstruction(SwapBuffersOpCodeParams().ToInstruction().Instruction);
}

std::future<std::shared_ptr<OpenGLBufferHandle>> OpenGLRenderer::SendGenBuffer()
{
    GenBufferOpCodeParams params(ng::make_unique<std::promise<std::shared_ptr<OpenGLBufferHandle>>>(), true);
    auto fut = params.Promise->get_future();

    PushResourceInstruction(params.ToInstruction().Instruction);
    params.AutoCleanup = false;

    return std::move(fut);
}

void OpenGLRenderer::SendDeleteBuffer(GLuint buffer)
{
    DeleteBufferOpCodeParams params(buffer);
    PushResourceInstruction(params.ToInstruction().Instruction);
}

std::future<std::shared_ptr<OpenGLBufferHandle>> OpenGLRenderer::SendBufferData(
        OpenGLInstructionHandler instructionHandler,
        std::shared_future<std::shared_ptr<OpenGLBufferHandle>> bufferHandle,
        GLenum target,
        GLsizeiptr size,
        std::shared_ptr<const void> dataHandle,
        GLenum usage)
{
    BufferDataOpCodeParams params(
                ng::make_unique<std::promise<std::shared_ptr<OpenGLBufferHandle>>>(),
                ng::make_unique<std::shared_future<std::shared_ptr<OpenGLBufferHandle>>>(bufferHandle),
                target,
                size,
                ng::make_unique<std::shared_ptr<const void>>(dataHandle),
                usage,
                true);
    auto fut = params.BufferDataPromise->get_future();

    PushInstruction(instructionHandler, params.ToInstruction().Instruction);
    params.AutoCleanup = false;

    return std::move(fut);
}

std::future<std::shared_ptr<OpenGLVertexArrayHandle>> OpenGLRenderer::SendGenVertexArray()
{
    GenVertexArrayOpCodeParams params(ng::make_unique<std::promise<std::shared_ptr<OpenGLVertexArrayHandle>>>(), true);
    auto fut = params.Promise->get_future();

    PushRenderingInstruction(params.ToInstruction().Instruction);
    params.AutoCleanup = false;

    return std::move(fut);
}

void OpenGLRenderer::SendDeleteVertexArray(GLuint vertexArray)
{
    DeleteVertexArrayOpCodeParams params(vertexArray);
    PushRenderingInstruction(params.ToInstruction().Instruction);
}

std::future<std::shared_ptr<OpenGLVertexArrayHandle>> OpenGLRenderer::SendSetVertexArrayLayout(
         std::shared_future<std::shared_ptr<OpenGLVertexArrayHandle>> vertexArrayHandle,
         VertexFormat format,
         std::map<VertexAttributeName,std::shared_future<std::shared_ptr<OpenGLBufferHandle>>> attributeBuffers,
         std::shared_future<std::shared_ptr<OpenGLBufferHandle>> indexBuffer)
{
    SetVertexArrayLayoutOpCodeParams params(
            ng::make_unique<std::promise<std::shared_ptr<OpenGLVertexArrayHandle>>>(),
            ng::make_unique<std::shared_future<std::shared_ptr<OpenGLVertexArrayHandle>>>(std::move(vertexArrayHandle)),
            ng::make_unique<VertexFormat>(std::move(format)),
            ng::make_unique<std::map<VertexAttributeName,std::shared_future<std::shared_ptr<OpenGLBufferHandle>>>>(std::move(attributeBuffers)),
            ng::make_unique<std::shared_future<std::shared_ptr<OpenGLBufferHandle>>>(std::move(indexBuffer)),
            true);
    auto fut = params.VertexArrayPromise->get_future();

    PushRenderingInstruction(params.ToInstruction().Instruction);
    params.AutoCleanup = false;

    return std::move(fut);
}

std::future<std::shared_ptr<OpenGLShaderHandle>> OpenGLRenderer::SendGenShader(GLenum shaderType)
{
    GenShaderOpCodeParams params(ng::make_unique<std::promise<std::shared_ptr<OpenGLShaderHandle>>>(), shaderType, true);
    auto fut = params.ShaderPromise->get_future();

    PushResourceInstruction(params.ToInstruction().Instruction);
    params.AutoCleanup = false;

    return std::move(fut);
}

void OpenGLRenderer::SendDeleteShader(GLuint shader)
{
    DeleteShaderOpCodeParams params(shader);
    PushResourceInstruction(params.ToInstruction().Instruction);
}

std::future<std::shared_ptr<OpenGLShaderHandle>> OpenGLRenderer::SendCompileShader(
        std::shared_future<std::shared_ptr<OpenGLShaderHandle>> shaderHandle,
        std::shared_ptr<const char> shaderSource)
{
    CompileShaderOpCodeParams params(
                ng::make_unique<std::promise<std::shared_ptr<OpenGLShaderHandle>>>(),
                ng::make_unique<std::shared_future<std::shared_ptr<OpenGLShaderHandle>>>(shaderHandle),
                ng::make_unique<std::shared_ptr<const char>>(shaderSource),
                true);
    auto fut = params.CompiledShaderPromise->get_future();

    PushResourceInstruction(params.ToInstruction().Instruction);
    params.AutoCleanup = false;

    return std::move(fut);
}

std::future<std::pair<bool,std::string>> OpenGLRenderer::SendGetShaderStatus(std::shared_future<std::shared_ptr<OpenGLShaderHandle>> shader)
{
    ShaderStatusOpCodeParams params(
                ng::make_unique<std::promise<std::pair<bool,std::string>>>(),
                ng::make_unique<std::shared_future<std::shared_ptr<OpenGLShaderHandle>>>(shader),
                true);
    auto fut = params.Promise->get_future();

    PushResourceInstruction(params.ToInstruction().Instruction);
    params.AutoCleanup = false;

    return std::move(fut);
}

std::future<std::shared_ptr<OpenGLShaderProgramHandle>> OpenGLRenderer::SendGenShaderProgram()
{
    GenShaderProgramOpCodeParams params(ng::make_unique<std::promise<std::shared_ptr<OpenGLShaderProgramHandle>>>(), true);
    auto fut = params.Promise->get_future();

    PushResourceInstruction(params.ToInstruction().Instruction);
    params.AutoCleanup = false;

    return std::move(fut);
}

void OpenGLRenderer::SendDeleteShaderProgram(GLuint program)
{
    DeleteShaderProgramOpCodeParams params(program);
    PushResourceInstruction(params.ToInstruction().Instruction);
}

std::future<std::shared_ptr<OpenGLShaderProgramHandle>> OpenGLRenderer::SendLinkProgram(
        std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>> programHandle,
        std::shared_future<std::shared_ptr<OpenGLShaderHandle>> vertexShaderHandle,
        std::shared_future<std::shared_ptr<OpenGLShaderHandle>> fragmentShaderHandle)
{
    LinkShaderProgramOpCodeParams params(
            ng::make_unique<std::promise<std::shared_ptr<OpenGLShaderProgramHandle>>>(),
            ng::make_unique<std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>>>(programHandle),
            ng::make_unique<std::shared_future<std::shared_ptr<OpenGLShaderHandle>>>(vertexShaderHandle),
            ng::make_unique<std::shared_future<std::shared_ptr<OpenGLShaderHandle>>>(fragmentShaderHandle),
            true);
    auto fut = params.LinkedProgramPromise->get_future();

    PushResourceInstruction(params.ToInstruction().Instruction);
    params.AutoCleanup = false;

    return std::move(fut);
}

std::future<std::pair<bool,std::string>> OpenGLRenderer::SendGetProgramStatus(std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>> program)
{
    ShaderProgramStatusOpCodeParams params(
                ng::make_unique<std::promise<std::pair<bool,std::string>>>(),
                ng::make_unique<std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>>>(program),
                true);
    auto fut = params.Promise->get_future();

    PushResourceInstruction(params.ToInstruction().Instruction);
    params.AutoCleanup = false;

    return std::move(fut);
}

void OpenGLRenderer::SendDrawVertexArray(
    std::shared_future<std::shared_ptr<OpenGLVertexArrayHandle>> vertexArray,
    std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>> program,
    GLenum mode,
    GLint firstVertexIndex,
    GLsizei vertexCount,
    bool isIndexed,
    ArithmeticType indexType)
{
    DrawVertexArrayOpCodeParams params(ng::make_unique<std::shared_future<std::shared_ptr<OpenGLVertexArrayHandle>>>(std::move(vertexArray)),
                                       ng::make_unique<std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>>>(std::move(program)),
                                       mode, firstVertexIndex, vertexCount, isIndexed, indexType, true);
    PushInstruction(RenderingInstructionHandler, params.ToInstruction().Instruction);
    params.AutoCleanup = false;
}

void OpenGLRenderer::Clear(bool color, bool depth, bool stencil)
{
    ClearOpCodeParams params(  (color   ? GL_COLOR_BUFFER_BIT   : 0)
                             | (depth   ? GL_DEPTH_BUFFER_BIT   : 0)
                             | (stencil ? GL_STENCIL_BUFFER_BIT : 0));

    PushRenderingInstruction(params.ToInstruction().Instruction);
}

void OpenGLRenderer::SwapBuffers()
{
    // make sure the buffer we're sending the swapbuffers commmand to
    // is the same that we will switch away from when swapping command queues
    std::lock_guard<std::recursive_mutex> lock(mRenderingThreadData->mCurrentWriteBufferMutex);

    SendSwapBuffers();
    SwapRenderingInstructionQueues();
}

std::shared_ptr<IStaticMesh> OpenGLRenderer::CreateStaticMesh()
{
    return std::make_shared<OpenGLStaticMesh>(shared_from_this());
}

std::shared_ptr<IShaderProgram> OpenGLRenderer::CreateShaderProgram()
{
    return std::make_shared<OpenGLShaderProgram>(shared_from_this());
}

static void* LoadProcOrDie(IGLContext& context, const char* procName)
{
    auto ext = context.GetProcAddress(procName);
    if (!ext)
    {
        throw std::runtime_error(std::string("Failed to load GL extension: ") + procName);
    }
    return ext;
}

// helper for declaring OpenGL functions
#define DeclareGLExtension(Type, ExtensionFunctionName) \
    static thread_local Type ExtensionFunctionName

// declarations of all the OpenGL functions used
static thread_local bool LoadedGLExtensions = false;
DeclareGLExtension(PFNGLGENBUFFERSPROC, glGenBuffers);
DeclareGLExtension(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
DeclareGLExtension(PFNGLBINDBUFFERPROC, glBindBuffer);
DeclareGLExtension(PFNGLBUFFERDATAPROC, glBufferData);
DeclareGLExtension(PFNGLMAPBUFFERPROC, glMapBuffer);
DeclareGLExtension(PFNGLUNMAPBUFFERPROC, glUnmapBuffer);
DeclareGLExtension(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
DeclareGLExtension(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);
DeclareGLExtension(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
DeclareGLExtension(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
DeclareGLExtension(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
DeclareGLExtension(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);
DeclareGLExtension(PFNGLCREATESHADERPROC, glCreateShader);
DeclareGLExtension(PFNGLDELETESHADERPROC, glDeleteShader);
DeclareGLExtension(PFNGLATTACHSHADERPROC, glAttachShader);
DeclareGLExtension(PFNGLDETACHSHADERPROC, glDetachShader);
DeclareGLExtension(PFNGLSHADERSOURCEPROC, glShaderSource);
DeclareGLExtension(PFNGLCOMPILESHADERPROC, glCompileShader);
DeclareGLExtension(PFNGLGETSHADERIVPROC, glGetShaderiv);
DeclareGLExtension(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
DeclareGLExtension(PFNGLCREATEPROGRAMPROC, glCreateProgram);
DeclareGLExtension(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
DeclareGLExtension(PFNGLUSEPROGRAMPROC, glUseProgram);
DeclareGLExtension(PFNGLLINKPROGRAMPROC, glLinkProgram);
DeclareGLExtension(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
DeclareGLExtension(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
DeclareGLExtension(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation);
DeclareGLExtension(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);

// clean up define
#undef DeclareGLExtension

// used by GetGLExtension
#ifndef STRINGIFY
    #define STRINGIFY(x) #x
#endif

// loads a single extension
#define GetGLExtension(context, Type, ExtensionFunctionName) \
    ExtensionFunctionName = reinterpret_cast<Type>(LoadProcOrDie(context, STRINGIFY(ExtensionFunctionName)));

// loads all extensions we need if they have not been loaded yet.
#define InitGLExtensions(context) \
    if (!LoadedGLExtensions) { \
        GetGLExtension(context, PFNGLGENBUFFERSPROC, glGenBuffers); \
        GetGLExtension(context, PFNGLDELETEBUFFERSPROC, glDeleteBuffers); \
        GetGLExtension(context, PFNGLBINDBUFFERPROC, glBindBuffer); \
        GetGLExtension(context, PFNGLBUFFERDATAPROC, glBufferData); \
        GetGLExtension(context, PFNGLMAPBUFFERPROC, glMapBuffer); \
        GetGLExtension(context, PFNGLUNMAPBUFFERPROC, glUnmapBuffer); \
        GetGLExtension(context, PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays); \
        GetGLExtension(context, PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays); \
        GetGLExtension(context, PFNGLBINDVERTEXARRAYPROC, glBindVertexArray); \
        GetGLExtension(context, PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer); \
        GetGLExtension(context, PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray); \
        GetGLExtension(context, PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray); \
        GetGLExtension(context, PFNGLCREATESHADERPROC, glCreateShader); \
        GetGLExtension(context, PFNGLDELETESHADERPROC, glDeleteShader); \
        GetGLExtension(context, PFNGLATTACHSHADERPROC, glAttachShader); \
        GetGLExtension(context, PFNGLDETACHSHADERPROC, glDetachShader); \
        GetGLExtension(context, PFNGLSHADERSOURCEPROC, glShaderSource); \
        GetGLExtension(context, PFNGLCOMPILESHADERPROC, glCompileShader); \
        GetGLExtension(context, PFNGLGETSHADERIVPROC, glGetShaderiv); \
        GetGLExtension(context, PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog); \
        GetGLExtension(context, PFNGLCREATEPROGRAMPROC, glCreateProgram); \
        GetGLExtension(context, PFNGLDELETEPROGRAMPROC, glDeleteProgram); \
        GetGLExtension(context, PFNGLUSEPROGRAMPROC, glUseProgram); \
        GetGLExtension(context, PFNGLLINKPROGRAMPROC, glLinkProgram); \
        GetGLExtension(context, PFNGLGETPROGRAMIVPROC, glGetProgramiv); \
        GetGLExtension(context, PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog); \
        GetGLExtension(context, PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation); \
        GetGLExtension(context, PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation); \
        LoadedGLExtensions = true; \
    }

// for instructions that act the same way for both threads.
static void HandleCommonInstruction(CommonOpenGLThreadData* threadData, const OpenGLInstruction& inst)
{
    switch (static_cast<OpenGLOpCode>(inst.OpCode))
    {
    case OpenGLOpCode::Clear: {
        ClearOpCodeParams params(inst);
        glClear(params.Mask);
    } break;
    case OpenGLOpCode::BufferData: {
        BufferDataOpCodeParams params(inst, true);

        glBindBuffer(params.Target, params.BufferHandle->get()->GetHandle());

        // initialize it with null, because glBufferData would make a useless copy of the data we pass it.
        glBufferData(params.Target, params.Size, nullptr, params.Usage);

        // write the initial data in the buffer
        if (params.DataHandle && *params.DataHandle)
        {
            void* bufferPtr = glMapBuffer(params.Target, GL_WRITE_ONLY);
            std::memcpy(bufferPtr, params.DataHandle->get(), params.Size);
            glUnmapBuffer(params.Target);
        }

        params.BufferDataPromise->set_value(params.BufferHandle->get());
    } break;
    case OpenGLOpCode::SwapBuffers: {
        threadData->mWindow->SwapBuffers();
    } break;
    default:
        RenderDebugPrintf("Invalid OpCode for %s: %u\n", threadData->mThreadName.c_str(), inst.OpCode);
    }
}

void OpenGLRenderingThreadEntry(RenderingOpenGLThreadData* threadData)
{
    threadData->mWindowManager->SetCurrentContext(threadData->mWindow, threadData->mContext);

    InitGLExtensions(*threadData->mContext);

    Profiler renderProfiler;

    size_t bufferToConsumeFrom = !RenderingOpenGLThreadData::InitialWriteBufferIndex;

    while (true)
    {
        bufferToConsumeFrom = !bufferToConsumeFrom;

        OpenGLInstructionLinearBuffer& instructionBuffer = threadData->mInstructionBuffers[bufferToConsumeFrom];

        // be ready to start reading from what's being written as soon as it's ready,
        // and release it back to the producer after.
        struct ConsumptionScope
        {
            OpenGLInstructionLinearBuffer& mInstructionBuffer;
            std::mutex& mConsumerMutex;
            std::mutex& mProducerMutex;

            ConsumptionScope(OpenGLInstructionLinearBuffer& instructionBuffer,
                             std::mutex& consumerMutex, std::mutex& producerMutex)
                : mInstructionBuffer(instructionBuffer)
                , mConsumerMutex(consumerMutex)
                , mProducerMutex(producerMutex)
            {
                // patiently wait until this buffer is available to consume
                mConsumerMutex.lock();
            }

            ~ConsumptionScope()
            {
                mInstructionBuffer.Reset();

                // makes this buffer available to the producer again
                mProducerMutex.unlock();
            }
        } consumptionScope(instructionBuffer,
                           threadData->mInstructionConsumerMutex[bufferToConsumeFrom],
                           threadData->mInstructionProducerMutex[bufferToConsumeFrom]);

        SizedOpenGLInstruction<OpenGLInstruction::MaxParams> sizedInst(SizedOpenGLInstruction<OpenGLInstruction::MaxParams>::NoInitTag{});
        OpenGLInstruction& inst = sizedInst.Instruction;

        renderProfiler.Start();

        while (instructionBuffer.PopInstruction(inst))
        {
            const OpenGLOpCode code = static_cast<OpenGLOpCode>(inst.OpCode);

            RenderDebugPrintf("Rendering thread processing %s\n", OpenGLOpCodeToString(code));

            switch (code)
            {
            case OpenGLOpCode::GenVertexArray: {
                GenVertexArrayOpCodeParams params(inst, true);
                GLuint handle;
                glGenVertexArrays(1, &handle);
                params.Promise->set_value(std::make_shared<OpenGLVertexArrayHandle>(threadData->mRenderer.shared_from_this(), handle));
            } break;
            case OpenGLOpCode::DeleteVertexArray: {
                DeleteVertexArrayOpCodeParams params(inst);
                glDeleteVertexArrays(1, &params.Handle);
            } break;
            case OpenGLOpCode::SetVertexArrayLayout: {
                SetVertexArrayLayoutOpCodeParams params(inst, true);

                const VertexFormat& format = *params.Format;

                glBindVertexArray(params.VertexArrayHandle->get()->GetHandle());

                for (const std::pair<VertexAttributeName, std::shared_future<std::shared_ptr<OpenGLBufferHandle>>>& attribBufferPair
                     : *params.AttributeBuffers)
                {
                    const VertexAttribute& attrib = format.Attributes.at(attribBufferPair.first);

                    glBindBuffer(GL_ARRAY_BUFFER, attribBufferPair.second.get()->GetHandle());
                    glVertexAttribPointer(ToGLAttributeIndex(attribBufferPair.first),
                                          attrib.Cardinality, ToGLArithmeticType(attrib.Type), attrib.IsNormalized,
                                          attrib.Stride, reinterpret_cast<void*>(attrib.Offset));
                    glEnableVertexAttribArray(ToGLAttributeIndex(attribBufferPair.first));
                }

                if (params.IndexBuffer && params.IndexBuffer->valid())
                {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, params.IndexBuffer->get()->GetHandle());
                }

                params.VertexArrayPromise->set_value(params.VertexArrayHandle->get());
            } break;
            case OpenGLOpCode::DrawVertexArray: {
                DrawVertexArrayOpCodeParams params(inst, true);

                glUseProgram(params.ProgramHandle->get()->GetHandle());

                glBindVertexArray(params.VertexArrayHandle->get()->GetHandle());

                if (params.IsIndexed)
                {
                    glDrawElements(params.Mode, params.VertexCount,
                                   ToGLArithmeticType(params.IndexType),
                                   reinterpret_cast<void*>(params.FirstVertexIndex * SizeOfArithmeticType(params.IndexType)));
                }
                else
                {
                    glDrawArrays(params.Mode, params.FirstVertexIndex, params.VertexCount);
                }
            } break;
            case OpenGLOpCode::Quit: {
                RenderProfilePrintf("Time spent rendering serverside in %s: %lfms\n", threadData->mThreadName.c_str(), renderProfiler.GetTotalTimeMS());
                RenderProfilePrintf("Average time spent rendering serverside in %s: %lfms\n", threadData->mThreadName.c_str(), renderProfiler.GetAverageTimeMS());
                return;
            } break;
            default: {
                HandleCommonInstruction(threadData, inst);
            } break;
            }
        }

        renderProfiler.Stop();
    }
}

void OpenGLResourceThreadEntry(ResourceOpenGLThreadData* threadData)
{
    threadData->mWindowManager->SetCurrentContext(threadData->mWindow, threadData->mContext);

    InitGLExtensions(*threadData->mContext);

    Profiler resourceProfiler;

    OpenGLInstructionRingBuffer& instructionBuffer = threadData->mInstructionBuffer;

    while (true)
    {
        // wait for an instruction to be available
        threadData->mConsumerSemaphore.wait();

        SizedOpenGLInstruction<OpenGLInstruction::MaxParams> sizedInst(SizedOpenGLInstruction<OpenGLInstruction::MaxParams>::NoInitTag{});
        OpenGLInstruction& inst = sizedInst.Instruction;

        // pop a single instruction from the buffer
        {
            std::lock_guard<std::recursive_mutex> instructionBufferLock(threadData->mInstructionBufferMutex);
            instructionBuffer.PopInstruction(inst);
        }

        resourceProfiler.Start();

        const OpenGLOpCode code = static_cast<OpenGLOpCode>(inst.OpCode);

        RenderDebugPrintf("Resource thread processing %s\n", OpenGLOpCodeToString(code));

        // now execute the instruction
        switch (code)
        {
        case OpenGLOpCode::GenBuffer: {
            GenBufferOpCodeParams params(inst, true);
            GLuint handle;
            glGenBuffers(1, &handle);
            params.Promise->set_value(std::make_shared<OpenGLBufferHandle>(threadData->mRenderer.shared_from_this(), handle));
        } break;
        case OpenGLOpCode::DeleteBuffer: {
            DeleteBufferOpCodeParams params(inst);
            glDeleteBuffers(1, &params.Handle);
        } break;
        case OpenGLOpCode::GenShader: {
            GenShaderOpCodeParams params(inst, true);
            params.ShaderPromise->set_value(std::make_shared<OpenGLShaderHandle>(threadData->mRenderer.shared_from_this(), glCreateShader(params.ShaderType)));
        } break;
        case OpenGLOpCode::DeleteShader: {
            DeleteShaderOpCodeParams params(inst);
            glDeleteShader(params.Handle);
        } break;
        case OpenGLOpCode::CompileShader: {
            CompileShaderOpCodeParams params(inst, true);

            const char* src = params.SourceHandle->get();
            GLuint handle = params.ShaderHandle->get()->GetHandle();

            glShaderSource(handle, 1, &src, NULL);
            glCompileShader(handle);

            params.CompiledShaderPromise->set_value(params.ShaderHandle->get());
        } break;
        case OpenGLOpCode::ShaderStatus: {
            ShaderStatusOpCodeParams params(inst, true);

            GLuint handle = params.Handle->get()->GetHandle();

            GLint status;
            glGetShaderiv(handle, GL_COMPILE_STATUS, &status);

            if (!status)
            {
                GLint logLength;
                glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLength);

                std::vector<char> log(logLength);
                glGetShaderInfoLog(handle, log.size(), NULL, log.data());

                params.Promise->set_value(std::make_pair<bool,std::string>(false, log.data()));
            }
            else
            {
                params.Promise->set_value(std::make_pair<bool,std::string>(true, "Compile Status OK"));
            }
        } break;
        case OpenGLOpCode::GenShaderProgram: {
            GenShaderProgramOpCodeParams params(inst, true);
            params.Promise->set_value(std::make_shared<OpenGLShaderProgramHandle>(threadData->mRenderer.shared_from_this(), glCreateProgram()));
        } break;
        case OpenGLOpCode::DeleteShaderProgram: {
            DeleteShaderProgramOpCodeParams params(inst);
            glDeleteProgram(params.Handle);
        } break;
        case OpenGLOpCode::LinkShaderProgram: {
            LinkShaderProgramOpCodeParams params(inst, true);

            GLuint programHandle = params.ShaderProgramHandle->get()->GetHandle();
            GLuint vertexShaderHandle = params.VertexShaderHandle->get()->GetHandle();
            GLuint fragmentShaderHandle = params.FragmentShaderHandle->get()->GetHandle();

            glAttachShader(programHandle, vertexShaderHandle);
            glAttachShader(programHandle, fragmentShaderHandle);

            glLinkProgram(programHandle);

            params.LinkedProgramPromise->set_value(params.ShaderProgramHandle->get());
        } break;
        case OpenGLOpCode::ShaderProgramStatus: {
            ShaderProgramStatusOpCodeParams params(inst, true);

            GLuint handle = params.Handle->get()->GetHandle();

            GLint status;
            glGetProgramiv(handle, GL_LINK_STATUS, &status);

            if (!status)
            {
                GLint logLength;
                glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &logLength);

                std::vector<char> log(logLength);
                glGetProgramInfoLog(handle, log.size(), NULL, log.data());

                params.Promise->set_value(std::make_pair<bool,std::string>(false, log.data()));
            }
            else
            {
                params.Promise->set_value(std::make_pair<bool,std::string>(true, "Link Status OK"));
            }
        } break;
        case OpenGLOpCode::Quit: {
            RenderProfilePrintf("Time spent loading resources serverside in %s: %lfms\n", threadData->mThreadName.c_str(), resourceProfiler.GetTotalTimeMS());
            RenderProfilePrintf("Average time spent loading resources serverside in %s: %lfms\n", threadData->mThreadName.c_str(), resourceProfiler.GetAverageTimeMS());
            return;
        } break;
        default: {
            HandleCommonInstruction(threadData, inst);
        } break;
        }

        resourceProfiler.Stop();
    }
}

// clean up defines
#undef GetGLExtension
#undef InitGLExtensions

} // end namespace ng
