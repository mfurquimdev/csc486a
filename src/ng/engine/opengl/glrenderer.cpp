#include "ng/engine/opengl/glrenderer.hpp"

#include "ng/engine/windowmanager.hpp"
#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"

#include "ng/engine/opengl/glinstruction.hpp"
#include "ng/engine/opengl/globject.hpp"

#include "ng/engine/profiler.hpp"
#include "ng/engine/debug.hpp"
#include "ng/engine/semaphore.hpp"
#include "ng/engine/memory.hpp"

#include <cstring>

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

struct CommonOpenGLThreadData
{
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

struct RenderingOpenGLThreadData : CommonOpenGLThreadData
{
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

struct ResourceOpenGLThreadData : CommonOpenGLThreadData
{
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

static void OpenGLRenderingThreadEntry(RenderingOpenGLThreadData* threadData);
static void OpenGLResourceThreadEntry(ResourceOpenGLThreadData* threadData);

ResourceHandle::InstanceID OpenGLRenderer::GenerateInstanceID()
{
    std::lock_guard<std::mutex> lock(mIDGenLock);

    static_assert(sizeof(mCurrentID) >= 8, "Assuming that we never realistically will need more than 2^64 ids");
    mCurrentID++;

    return mCurrentID;
}


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

ResourceHandle OpenGLRenderer::SendGenBuffer()
{
    // grab a new promise so we can fill it up.
    GenBufferOpCodeParams params(ng::make_unique<std::promise<OpenGLBuffer>>(), true);
    ResourceHandle res(GenerateInstanceID(), GLSharedFutureToBufferClassID,
                       std::make_shared<std::shared_future<OpenGLBuffer>>(
                           params.BufferPromise->get_future()));

    PushResourceInstruction(params.ToInstruction().Instruction);
    params.AutoCleanup = false;

    return res;
}

void OpenGLRenderer::SendDeleteBuffer(GLuint buffer)
{
    DeleteBufferOpCodeParams params(buffer);
    PushResourceInstruction(params.ToInstruction().Instruction);
}

void OpenGLRenderer::SendBufferData(
        OpenGLInstructionHandler instructionHandler,
        ResourceHandle bufferHandle,
        GLenum target,
        GLsizeiptr size,
        std::shared_ptr<const void> dataHandle,
        GLenum usage)
{
    BufferDataOpCodeParams params(ng::make_unique<ResourceHandle>(bufferHandle),
                                  target,
                                  size,
                                  ng::make_unique<std::shared_ptr<const void>>(dataHandle),
                                  usage,
                                  true);
    PushInstruction(instructionHandler, params.ToInstruction().Instruction);
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

constexpr ResourceHandle::ClassID OpenGLRenderer::GLSharedFutureToBufferClassID;

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

// clean up define
#undef DeclareGLExtension

// used by GetGLExtension
#ifndef STRINGIFY
    #define STRINGIFY(x) #x
#endif

// loads a single extension
#define GetGLExtension(context, Type, ExtensionFunctionName) \
    ExtensionFunctionName = (Type) LoadProcOrDie(context, STRINGIFY(ExtensionFunctionName))

// loads all extensions we need if they have not been loaded yet.
#define InitGLExtensions(context) \
    if (!LoadedGLExtensions) { \
        GetGLExtension(context, PFNGLGENBUFFERSPROC,    glGenBuffers); \
        GetGLExtension(context, PFNGLDELETEBUFFERSPROC, glDeleteBuffers); \
        GetGLExtension(context, PFNGLBINDBUFFERPROC,    glBindBuffer); \
        GetGLExtension(context, PFNGLBUFFERDATAPROC,    glBufferData); \
        GetGLExtension(context, PFNGLMAPBUFFERPROC,     glMapBuffer); \
        GetGLExtension(context, PFNGLUNMAPBUFFERPROC,   glUnmapBuffer); \
        LoadedGLExtensions = true; \
    }

// for instructions that act the same way for both threads.
static void HandleCommonInstruction(CommonOpenGLThreadData* threadData, const OpenGLInstruction& inst)
{
    switch (static_cast<OpenGLOpCode::OpCodeID>(inst.OpCode))
    {
    case OpenGLOpCode::Clear: {
        ClearOpCodeParams params(inst);
        glClear(params.Mask);
    } break;
    case OpenGLOpCode::BufferData: {
        BufferDataOpCodeParams params(inst, true);

        glBindBuffer(params.Target, params.BufferFuture->get().GetHandle());

        // initialize it with null, because glBufferData would make a useless copy of the data we pass it.
        glBufferData(params.Target, params.Size, nullptr, params.Usage);

        // write the initial data in the buffer
        if (params.DataHandle && *params.DataHandle)
        {
            void* bufferPtr = glMapBuffer(params.Target, GL_WRITE_ONLY);
            std::memcpy(bufferPtr, params.DataHandle->get(), params.Size);
            glUnmapBuffer(params.Target);
        }
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
            RenderDebugPrintf("Rendering thread processing %s\n", OpenGLOpCode(static_cast<OpenGLOpCode::OpCodeID>(inst.OpCode)).ToString());
            switch (static_cast<OpenGLOpCode::OpCodeID>(inst.OpCode))
            {
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

        RenderDebugPrintf("Resource thread processing %s\n", OpenGLOpCode(static_cast<OpenGLOpCode::OpCodeID>(inst.OpCode)).ToString());

        // now execute the instruction
        switch (static_cast<OpenGLOpCode::OpCodeID>(inst.OpCode))
        {
        case OpenGLOpCode::GenBuffer: {
            GenBufferOpCodeParams params(inst, true);
            GLuint handle;
            glGenBuffers(1, &handle);
            params.BufferPromise->set_value(OpenGLBuffer(threadData->mRenderer.shared_from_this(), handle));
        } break;
        case OpenGLOpCode::DeleteBuffer: {
            DeleteBufferOpCodeParams params(inst);
            glDeleteBuffers(1, &params.Buffer);
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
