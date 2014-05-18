#include "ng/engine/renderer.hpp"

#include "ng/engine/windowmanager.hpp"
#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"
#include "ng/engine/profiler.hpp"
#include "ng/engine/debug.hpp"
#include "ng/engine/vertexformat.hpp"
#include "ng/engine/memory.hpp"
#include "ng/engine/dynamicmesh.hpp"
#include "ng/engine/semaphore.hpp"

#include <GL/gl.h>

#include <thread>
#include <cstdint>
#include <vector>
#include <limits>
#include <cstring>
#include <algorithm>
#include <cstddef>
#include <future>
#include <map>

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

struct OpenGLInstruction
{
    static const size_t MaxParams = 16;

    std::uint32_t OpCode;
    size_t NumParams;
    std::uintptr_t Params[1]; // note: flexible array sized by NumParams

    size_t GetByteSize() const
    {
        return GetSizeForNumParams(NumParams);
    }

    static size_t GetMaxByteSize()
    {
        return GetSizeForNumParams(MaxParams);
    }

    constexpr static size_t GetSizeForNumParams(size_t params)
    {
        return sizeof(OpenGLInstruction) + (params - 1) * sizeof(Params[0]);
    }
};

static_assert(std::is_pod<OpenGLInstruction>::value, "GPUInstructions must be plain old data.");

struct OpenGLInstructionBuffer
{
    std::vector<char> mBuffer;
    size_t mReadHead;
    size_t mWriteHead;

    OpenGLInstructionBuffer(size_t commandBufferSize)
        : mBuffer(commandBufferSize)
        , mReadHead(0)
        , mWriteHead(0)
    { }

    // Returns true on success, false on failure.
    // On failure, the instruction is not pushed. Failure happens due to not enough memory.
    // Failure can be handled either by increasing commandBufferSize, or by flushing the instructions.
    bool PushInstruction(const OpenGLInstruction& inst)
    {
        size_t bytesToWrite = inst.GetByteSize();

        // is there room to write it?
        if (bytesToWrite <= mBuffer.size() - mWriteHead)
        {
            // then write it and move the write head up
            std::memcpy(&mBuffer[mWriteHead], &inst, bytesToWrite);
            mWriteHead += bytesToWrite;
        }
        else
        {
            // Possible improvement: resize the buffer?
            return false;
        }

        return true;
    }

    bool CanPopInstruction() const
    {
        // can keep popping until the read catches up with the write
        return mReadHead != mWriteHead;
    }

    // returns true and writes to inst if there was something to pop that was popped.
    // returns false otherwise.
    // nice pattern: while (PopInstruction(inst))
    bool PopInstruction(OpenGLInstruction& inst)
    {
        if (!CanPopInstruction())
        {
            return false;
        }

        // first read the header: OpCode and NumParams
        const size_t offsetOfParams = offsetof(OpenGLInstruction, Params);
        size_t headerSize = offsetOfParams;
        char* instdata = reinterpret_cast<char*>(&inst);

        // copy the header into the data structure.
        std::memcpy(instdata, &mBuffer[mReadHead], headerSize);
        mReadHead += headerSize;

        // next, read the params
        size_t paramsSize = sizeof(OpenGLInstruction().Params[0]) * inst.NumParams;
        std::memcpy(instdata + offsetOfParams, &mBuffer[mReadHead], paramsSize);
        mReadHead += paramsSize;

        return true;
    }

    void Reset()
    {
        mReadHead = mWriteHead = 0;
    }
};

class GL3Renderer;

using GL3Buffers = std::vector<GLuint>;

class GL3DynamicMesh : public IDynamicMesh
{
public:
    std::shared_ptr<GL3Renderer> mRenderer;
    VertexFormat mVertexFormat;

    std::future<std::shared_ptr<GL3Buffers>> mFutureBuffers;
    std::shared_ptr<GL3Buffers> mBuffers;

    std::shared_ptr<GL3Buffers>& GetBuffers()
    {
        if (!mBuffers)
        {
            mBuffers = mFutureBuffers.get();
        }
        return mBuffers;
    }

    GL3DynamicMesh(std::shared_ptr<GL3Renderer> renderer);
    ~GL3DynamicMesh();

    void Init(const VertexFormat& format,
              unique_deleted_ptr<const void> vertexData,
              size_t vertexDataSize,
              unique_deleted_ptr<const void> indexData,
              size_t elementDataSize) override;
};

struct CommonOpenGLThreadData
{
    CommonOpenGLThreadData(
            const std::string& threadName,
            const std::shared_ptr<IWindowManager>& windowManager,
            const std::shared_ptr<IWindow>& window,
            const std::shared_ptr<IGLContext>& context,
            GL3Renderer& renderer)
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

    GL3Renderer& mRenderer;
};

struct RenderingOpenGLThreadData : CommonOpenGLThreadData
{
    static const size_t InitialWriteBufferIndex = 0;

    RenderingOpenGLThreadData(
            const std::string& threadName,
            const std::shared_ptr<IWindowManager>& windowManager,
            const std::shared_ptr<IWindow>& window,
            const std::shared_ptr<IGLContext>& context,
            GL3Renderer& renderer,
            size_t instructionBufferSize)
        : mThreadName(threadName)
        , mWindowManager(windowManager)
        , mWindow(window)
        , mContext(context)
        , mRenderer(renderer)
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
    OpenGLInstructionBuffer mInstructionBuffers[2];
    std::mutex mInstructionProducerMutex[2];
    std::mutex mInstructionConsumerMutex[2];

    size_t mCurrentWriteBufferIndex;
    std::recursive_mutex mCurrentWriteBufferIndexMutex;

};

struct ResourceOpenGLThreadData : CommonOpenGLThreadData
{
    using CommonOpenGLThreadData::CommonOpenGLThreadData;

    // the resource loading thread just loads resources as soon as it can all the time.
    // it knows to keep loading resources as long as the semaphore is up.

    semaphore mConsumerSemaphore;
};

enum class OpenGLOpCode : decltype(OpenGLInstruction().OpCode)
{
    Clear,         // params: (GLbitfield mask)
    GenBuffers,    // params: (GLsizei n, promise<shared_ptr<GL3Buffers>>*)
                   // note: must fulfill the promise of n buffers
    DeleteBuffers, // params: (GLsizei n, GLuint buffers[])
                   // note: must delete[] buffers
    SwapBuffers,   // special instruction to signal a buffer swap.
    Quit           // special instruction to stop the graphics thread.
};

template<size_t NParams>
struct SizedOpenGLInstruction
{
    static_assert(NParams <= OpenGLInstruction::MaxParams, "Limit of OpenGL instruction parameters should be respected.");

    struct NoInitTag { };

    union
    {
        char InstructionData[OpenGLInstruction::GetSizeForNumParams(NParams)];
        OpenGLInstruction Instruction;
    };

    SizedOpenGLInstruction(NoInitTag){ }

    SizedOpenGLInstruction()
    {
        Instruction.NumParams = NParams;
    }

    SizedOpenGLInstruction(OpenGLOpCode code)
        : SizedOpenGLInstruction()
    {
        Instruction.OpCode = static_cast<decltype(Instruction.OpCode)>(code);
    }
};

static void OpenGLRenderingThreadEntry(CommonOpenGLThreadData* threadData);
static void OpenGLResourceThreadEntry(CommonOpenGLThreadData* threadData);

class GL3Renderer: public IRenderer, public std::enable_shared_from_this<GL3Renderer>
{
public:
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mRenderingContext;
    std::shared_ptr<IGLContext> mResourceContext;

    std::unique_ptr<CommonOpenGLThreadData> mRenderingThreadData;
    std::unique_ptr<ResourceOpenGLThreadData> mResourceThreadData;

    std::thread mRenderingThread;
    std::thread mResourceThread;

    static const size_t OneMB = 0x100000;
    static const size_t RenderingCommandBufferSize = OneMB;
    static const size_t ResourceCommandBufferSize = OneMB;

    GL3Renderer(
            const std::shared_ptr<IWindowManager>& windowManager,
            const std::shared_ptr<IWindow>& window)
        : mWindow(window)
        , mRenderingContext(windowManager->CreateContext(window->GetVideoFlags(), nullptr))
        , mResourceContext(windowManager->CreateContext(window->GetVideoFlags(), mRenderingContext))
    {
        mRenderingThreadData.reset(new CommonOpenGLThreadData(
                                       "OpenGL_Rendering",
                                       windowManager,
                                       window,
                                       mRenderingContext,
                                       *this,
                                       RenderingCommandBufferSize));

        mResourceThreadData.reset(new ResourceOpenGLThreadData(
                                      "OpenGL_Resources",
                                      windowManager,
                                      window,
                                      mResourceContext,
                                      *this,
                                      ResourceCommandBufferSize));

        mRenderingThread = std::thread(OpenGLRenderingThreadEntry, mRenderingThreadData.get());
        mResourceThread = std::thread(OpenGLResourceThreadEntry, mResourceThreadData.get());
    }

    void PushInstruction(CommonOpenGLThreadData& threadData, const OpenGLInstruction& inst)
    {
        // lock the index so the current write buffer won't change under our feet.
        std::lock_guard<std::recursive_mutex> indexLock(threadData.mCurrentWriteBufferIndexMutex);
        auto currentIndex = threadData.mCurrentWriteBufferIndex;

        RenderDebugPrintf("Pushing instruction to %zu\n", currentIndex);

        if (!threadData.mInstructionBuffers[currentIndex].PushInstruction(inst))
        {
            // TODO: Need to flush the queue and try again? or maybe just report an error and give up?
            // Might also be a good solution to dynamically resize the queue
            throw std::overflow_error("Command buffer too small for instructions. Increase size or improve OpenGLInstructionRingBuffer::PushInstruction()");
        }
    }

    void SendSwapBuffers(CommonOpenGLThreadData& threadData)
    {
        SizedOpenGLInstruction<0> sizedInst(OpenGLOpCode::SwapBuffers);
        PushInstruction(threadData, sizedInst.Instruction);
    }

    void SendQuit(CommonOpenGLThreadData& threadData)
    {
        SizedOpenGLInstruction<0> sizedInst(OpenGLOpCode::Quit);
        PushInstruction(threadData, sizedInst.Instruction);
    }

    std::future<std::shared_ptr<GL3Buffers>> SendGenBuffers(ResourceOpenGLThreadData& threadData, GLsizei n)
    {
        SizedOpenGLInstruction<2> sizedInst(OpenGLOpCode::GenBuffers);
        sizedInst.Instruction.Params[0] = n;

        // grab a new promise so we can fill it up.
        auto promisePtr = ng::make_unique<std::promise<std::shared_ptr<GL3Buffers>>>();
        auto fut = promisePtr->get_future();
        sizedInst.Instruction.Params[1] = (std::uintptr_t) promisePtr.get();

        PushInstruction(threadData, sizedInst.Instruction);

        promisePtr.release();
        return fut;
    }

    void SendDeleteBuffers(ResourceOpenGLThreadData& threadData, size_t n, GLuint* buffers)
    {
        SizedOpenGLInstruction<2> sizedInst(OpenGLOpCode::DeleteBuffers);
        sizedInst.Instruction.Params[0] = n;

        // grab a new vector so we can fill it up
        std::unique_ptr<GLuint[]> buffersList(new GLuint[n]);
        std::copy(buffersList.get(), buffersList.get() + n, buffers);
        sizedInst.Instruction.Params[1] = (std::uintptr_t) buffersList.get();

        PushInstruction(threadData, sizedInst.Instruction);
        buffersList.release();
    }

    void SwapCommandQueues(CommonOpenGLThreadData& threadData)
    {
        // make sure nobody else is relying on the current write buffer to stay the same.
        std::lock_guard<std::recursive_mutex> indexLock(threadData.mCurrentWriteBufferIndexMutex);
        auto finishedWriteIndex = threadData.mCurrentWriteBufferIndex;

        // must have production rights to be able to start writing to the other thread.
        threadData.mInstructionProducerMutex[!finishedWriteIndex].lock();

        // start writing to the other thread.
        threadData.mCurrentWriteBufferIndex = !threadData.mCurrentWriteBufferIndex;

        // allow consumer to begin reading what was just written
        mRenderingThreadData->mInstructionConsumerMutex[finishedWriteIndex].unlock();

        RenderDebugPrintf("SwapCommandQueues set mCurrentWriteBufferIndex to %zu\n", threadData.mCurrentWriteBufferIndex);
    }

    ~GL3Renderer()
    {
        std::unique_lock<std::recursive_mutex> resourceWriteIndexLock(mResourceThreadData->mCurrentWriteBufferIndexMutex);
        auto resourceWrittenBufferIndex = mResourceThreadData->mCurrentWriteBufferIndex;
        SendQuit(*mResourceThreadData);
        SwapCommandQueues(*mResourceThreadData);
        resourceWriteIndexLock.unlock();
        mResourceThreadData->mInstructionConsumerMutex[resourceWrittenBufferIndex].unlock();

        std::unique_lock<std::recursive_mutex> renderingWriteIndexLock(mRenderingThreadData->mCurrentWriteBufferIndexMutex);
        auto renderingWrittenBufferIndex = mRenderingThreadData->mCurrentWriteBufferIndex;
        SendQuit(*mRenderingThreadData);
        SwapCommandQueues(*mRenderingThreadData);
        renderingWriteIndexLock.unlock();
        mRenderingThreadData->mInstructionConsumerMutex[renderingWrittenBufferIndex].unlock();

        mResourceThread.join();
        mRenderingThread.join();
    }

    void Clear(bool color, bool depth, bool stencil) override
    {
        RenderDebugPrintf("Begin GL3Renderer::Clear()\n");

        SizedOpenGLInstruction<1> sizedInst(OpenGLOpCode::Clear);
        sizedInst.Instruction.Params[0] = (color   ? GL_COLOR_BUFFER_BIT   : 0)
                                        | (depth   ? GL_DEPTH_BUFFER_BIT   : 0)
                                        | (stencil ? GL_STENCIL_BUFFER_BIT : 0);
        PushInstruction(*mRenderingThreadData, sizedInst.Instruction);

        RenderDebugPrintf("End GL3Renderer::Clear()\n");
    }

    void SwapBuffers() override
    {
        RenderDebugPrintf("Begin GL3Renderer::SwapBuffers()\n");

        // make sure the buffer we're sending the swapbuffers commmand to
        // is the same that we will switch away from when swapping command queues
        std::lock_guard<std::recursive_mutex> lock(mRenderingThreadData->mCurrentWriteBufferIndexMutex);

        SendSwapBuffers(*mRenderingThreadData);
        SwapCommandQueues(*mRenderingThreadData);

        RenderDebugPrintf("End GL3Renderer::SwapBuffers()\n");
    }

    std::shared_ptr<IDynamicMesh> CreateDynamicMesh() override
    {
        return std::make_shared<GL3DynamicMesh>(shared_from_this());
    }
};

GL3Buffers::GL3Buffers(std::shared_ptr<GL3Renderer> renderer, size_t n)
    : mRenderer(std::move(renderer))
    , mBufferIDs(n, 0)
{ }

GL3Buffers::~GL3Buffers()
{
    mRenderer->SendDeleteBuffers(*mRenderer->mResourceThreadData, mBufferIDs.size(), mBufferIDs.data());
}

GL3DynamicMesh::GL3DynamicMesh(std::shared_ptr<GL3Renderer> renderer)
    : mRenderer(std::move(renderer))
{
    mFutureBuffers = mRenderer->SendGenBuffers(*mRenderer->mResourceThreadData, 2);
}

GL3DynamicMesh::~GL3DynamicMesh()
{
    // force the future buffers to have been received.
    GetBuffers();
    // now delete them
     mRenderer->SendDeleteBuffers(*mRenderer->mResourceThreadData, mBuffers->size(), mBuffers->data());
}

void GL3DynamicMesh::Init(const VertexFormat& format,
          unique_deleted_ptr<const void> vertexData,
          size_t vertexDataSize,
          unique_deleted_ptr<const void> indexData,
          size_t elementDataSize)
{
    mVertexFormat = format;
    // TODO: Implement
}

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window)
{
    return std::shared_ptr<GL3Renderer>(new GL3Renderer(windowManager, window));
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

// used by GetExtension
#ifndef STRINGIFY
    #define STRINGIFY(x) #x
#endif

// grabs a single extension as a local variable
#define GetExtension(context, Type, ExtensionFunctionName) \
    Type ExtensionFunctionName = (Type) LoadProcOrDie(context, STRINGIFY(ExtensionFunctionName))

// get all necessary GL extensions and put them into the enclosing scope
#define GetAllExtensions(ctx) \
    GetExtension(ctx, PFNGLGENBUFFERSPROC, glGenBuffers); \
    GetExtension(ctx, PFNGLDELETEBUFFERSPROC, glDeleteBuffers)

// clean up GetExtension define
#undef GetExtension

void OpenGLRenderingThreadEntry(RenderingOpenGLThreadData* threadData)
{
    threadData->mWindowManager->SetCurrentContext(threadData->mWindow, threadData->mContext);

    GetAllExtensions(threadData->mContext);

    Profiler renderProfiler;

    size_t bufferToConsumeFrom = !RenderingOpenGLThreadData::InitialWriteBufferIndex;

    while (true)
    {
        bufferToConsumeFrom = !bufferToConsumeFrom;

        OpenGLInstructionBuffer& instructionBuffer = threadData->mInstructionBuffers[bufferToConsumeFrom];

        RenderDebugPrintf("%s is waiting for consumer mutex %zu to start rendering.\n", threadData->mThreadName.c_str(), bufferToConsumeFrom);
        // be ready to start reading from what's being written as soon as it's ready,
        // and release it back to the producer after.
        struct ConsumptionScope
        {
            OpenGLInstructionBuffer& mInstructionBuffer;
            std::mutex& mConsumerMutex;
            std::mutex& mProducerMutex;

            ConsumptionScope(OpenGLInstructionBuffer& instructionBuffer,
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

        RenderDebugPrintf("Started reading instruction buffer %zu in %s\n", bufferToConsumeFrom, threadData->mThreadName.c_str());
        renderProfiler.Start();

        while (instructionBuffer.PopInstruction(inst))
        {
            switch (static_cast<OpenGLOpCode>(inst.OpCode))
            {
            case OpenGLOpCode::Clear: {
                RenderDebugPrintf("Doing OpenGLOpCode::Clear\n");
                GLbitfield mask = inst.Params[0];
                glClear(mask);
                RenderDebugPrintf("Done OpenGLOpCode::Clear\n");
            } break;
            case OpenGLOpCode::GenBuffers: {
                RenderDebugPrintf("Doing OpenGLOpCode::GenBuffers");
                GLsizei n = inst.Params[0];
                auto pro = (std::promise<std::shared_ptr<GL3Buffers>>*) inst.Params[1];
                auto buffers = std::make_shared<GL3Buffers>(n);
                glGenBuffers(n, buffers->mBufferIDs.data());
                pro->set_value(buffers);
                RenderDebugPrintf("Done OpenGLOpCode::GenBuffers");
            } break;
            case OpenGLOpCode::DeleteBuffers: {
                RenderDebugPrintf("Doing OpenGLOpCode::DeleteBuffers");
                GLsizei n = inst.Params[0];
                GLuint* buffers = (GLuint*) inst.Params[1];
                glDeleteBuffers(n, buffers);
                delete[] buffers;
                RenderDebugPrintf("Done OpenGLOpCode::DeleteBuffers");
            } break;
            case OpenGLOpCode::SwapBuffers: {
                RenderDebugPrintf("Doing OpenGLOpCode::SwapBuffers\n");
                threadData->mWindow->SwapBuffers();
                RenderDebugPrintf("Done OpenGLOpCode::SwapBuffers\n");
            } break;
            case OpenGLOpCode::Quit: {
                RenderProfilePrintf("Time spent rendering serverside in %s: %lfms\n", threadData->mThreadName.c_str(), renderProfiler.GetTotalTimeMS());
                RenderProfilePrintf("Average time spent rendering serverside in %s: %lfms\n", threadData->mThreadName.c_str(), renderProfiler.GetAverageTimeMS());
                return;
            } break;
            }
        }

        renderProfiler.Stop();
        RenderDebugPrintf("Finished reading instruction buffer %zu in %s\n", bufferToConsumeFrom, threadData->mThreadName.c_str());
    }
}

void OpenGLResourceThreadEntry(ResourceOpenGLThreadData* threadData)
{
    threadData->mWindowManager->SetCurrentContext(threadData->mWindow, threadData->mContext);

    GetAllExtensions(threadData->mContext);

    while (true)
    {

    }
}

} // end namespace ng
