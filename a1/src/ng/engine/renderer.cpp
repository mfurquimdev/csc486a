#include "ng/engine/renderer.hpp"

#include "ng/engine/windowmanager.hpp"
#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"
#include "ng/engine/profiler.hpp"
#include "ng/engine/debug.hpp"
#include "ng/engine/vertexformat.hpp"

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_PROTOTYPES

#include <thread>
#include <cstdint>
#include <vector>
#include <limits>
#include <cstring>
#include <algorithm>
#include <cstddef>
#include <future>
#include <set>

#if 0
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

class GL3Buffer
{
public:
};

struct CommonOpenGLThreadData
{
    static const size_t InitialWriteBufferIndex = 0;

    CommonOpenGLThreadData(
            const std::string& threadName,
            const std::shared_ptr<IWindowManager>& windowManager,
            const std::shared_ptr<IWindow>& window,
            const std::shared_ptr<IGLContext>& context,
            size_t instructionBufferSize)
            : mThreadName(threadName)
            , mWindowManager(windowManager)
            , mWindow(window)
            , mContext(context)
            , mInstructionBuffers{instructionBufferSize, instructionBufferSize}
            , mCurrentWriteBufferIndex(InitialWriteBufferIndex)
    {
        // consumer mutexes are initially locked, since nothing has been produced yet.
        mInstructionConsumerMutex[0].lock();
        mInstructionConsumerMutex[1].lock();

        // the initial write buffer's producer mutex is locked since it's initially producing
        mInstructionProducerMutex[InitialWriteBufferIndex].lock();
    }

    std::string mThreadName;

    std::shared_ptr<IWindowManager> mWindowManager;
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mContext;

    OpenGLInstructionBuffer mInstructionBuffers[2];
    std::mutex mInstructionProducerMutex[2];
    std::mutex mInstructionConsumerMutex[2];

    size_t mCurrentWriteBufferIndex;
    std::recursive_mutex mCurrentWriteBufferIndexMutex;
};

struct ResourceOpenGLThreadData : CommonOpenGLThreadData
{
    using BufferPromise = std::promise<std::shared_ptr<GL3Buffer>>;

    using CommonOpenGLThreadData::CommonOpenGLThreadData;

    std::mutex mBufferPromisesMutex;
    std::set<std::unique_ptr<BufferPromise>> mBufferPromises;
};

enum class OpenGLOpCode : decltype(OpenGLInstruction().OpCode)
{
    Clear, // params: same as glClear.
    GenBuffers, // params: n, promise unique_ptr pointer
    SwapBuffers, // special instruction to signal a buffer swap.
    Quit   // special instruction to stop the graphics thread.
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

static void OpenGLThreadEntry(CommonOpenGLThreadData* threadData)
{
    threadData->mWindowManager->SetCurrentContext(threadData->mWindow, threadData->mContext);

    // used by GetExtension
#ifndef STRINGIFY
    #define STRINGIFY(x) #x
#endif

    // for convenience
#define GetExtension(Type, ExtensionFunctionName) \
    do { \
        ExtensionFunctionName = (Type) threadData->mContext->GetProcAddress(STRINGIFY(ExtensionFunctionName)); \
        if (!ExtensionFunctionName) { \
            throw std::runtime_error("Failed to load GL extension: " STRINGIFY(ExtensionFunctionName)); \
        } \
    } while (0)

    // get all necessary GL extensions
    GetExtension(PFNGLGENBUFFERSPROC, glGenBuffers);

    // clean up GetExtension define
#undef GetExtension

    Profiler renderProfiler;

    size_t bufferToConsumeFrom = !CommonOpenGLThreadData::InitialWriteBufferIndex;

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
                GLsizei n = inst.Params[0];
                glGenBuffers();
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

class GL3Renderer: public IRenderer
{
public:
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mRenderingContext;
    std::shared_ptr<IGLContext> mResourceContext;

    CommonOpenGLThreadData mRenderingThreadData;
    CommonOpenGLThreadData mResourceThreadData;

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
        , mRenderingThreadData("OpenGL_Rendering",
                               windowManager,
                               window,
                               mRenderingContext,
                               RenderingCommandBufferSize)
        , mResourceThreadData("OpenGL_Resources",windowManager,
                              window,
                              mResourceContext,
                              ResourceCommandBufferSize)
        , mRenderingThread(OpenGLThreadEntry, &mRenderingThreadData)
        , mResourceThread(OpenGLThreadEntry, &mResourceThreadData)
    { }

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

    std::future<std::shared_ptr<GL3Buffer>> SendGenBuffers(CommonOpenGLThreadData& threadData, GLsizei n)
    {
        ResourceOpenGLThreadData& resourceData = static_cast<ResourceOpenGLThreadData&>(threadData);

        SizedOpenGLInstruction<2> sizedInst(OpenGLOpCode::GenBuffers);
        sizedInst.Instruction.Params[0] = n;

        // grab a new promise so we can fill it up.
        std::lock_guard<std::mutex> bufferPromiseLock(resourceData.mBufferPromisesMutex);
        auto promiseIt = resourceData.mBufferPromises.emplace(new ResourceOpenGLThreadData::BufferPromise());
        sizedInst.Instruction.Params[1] = &*promiseIt.first;

        PushInstruction(threadData, sizedInst.Instruction);
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
        mRenderingThreadData.mInstructionConsumerMutex[finishedWriteIndex].unlock();

        RenderDebugPrintf("SwapCommandQueues set mCurrentWriteBufferIndex to %zu\n", threadData.mCurrentWriteBufferIndex);
    }

    ~GL3Renderer()
    {
        std::unique_lock<std::recursive_mutex> resourceWriteIndexLock(mResourceThreadData.mCurrentWriteBufferIndexMutex);
        auto resourceWrittenBufferIndex = mResourceThreadData.mCurrentWriteBufferIndex;
        SendQuit(mResourceThreadData);
        SwapCommandQueues(mResourceThreadData);
        resourceWriteIndexLock.unlock();
        mResourceThreadData.mInstructionConsumerMutex[resourceWrittenBufferIndex].unlock();

        std::unique_lock<std::recursive_mutex> renderingWriteIndexLock(mRenderingThreadData.mCurrentWriteBufferIndexMutex);
        auto renderingWrittenBufferIndex = mRenderingThreadData.mCurrentWriteBufferIndex;
        SendQuit(mRenderingThreadData);
        SwapCommandQueues(mRenderingThreadData);
        renderingWriteIndexLock.unlock();
        mRenderingThreadData.mInstructionConsumerMutex[renderingWrittenBufferIndex].unlock();

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
        PushInstruction(mRenderingThreadData, sizedInst.Instruction);

        RenderDebugPrintf("End GL3Renderer::Clear()\n");
    }

    void SwapBuffers() override
    {
        RenderDebugPrintf("Begin GL3Renderer::SwapBuffers()\n");

        // make sure the buffer we're sending the swapbuffers commmand to
        // is the same that we will switch away from when swapping command queues
        std::lock_guard<std::recursive_mutex> lock(mRenderingThreadData.mCurrentWriteBufferIndexMutex);

        SendSwapBuffers(mRenderingThreadData);
        SwapCommandQueues(mRenderingThreadData);

        RenderDebugPrintf("End GL3Renderer::SwapBuffers()\n");
    }

    std::shared_ptr<IDynamicMesh> CreateDynamicMesh() override;
};

class GL3DynamicMesh : public IDynamicMesh
{
    std::shared_ptr<GL3Renderer> mRenderer;
    VertexFormat mVertexFormat;

public:

    GL3DynamicMesh(std::shared_ptr<GL3Renderer> renderer)
        : mRenderer(renderer)
    {
        mRenderer->SendGenBuffers(mRenderer->mResourceThreadData, 1);
    }

    void Init(const VertexFormat& format,
              unique_deleted_ptr<const void> vertexData,
              size_t vertexDataSize,
              unique_deleted_ptr<const void> indexData,
              size_t elementDataSize) override
    {
        mVertexFormat = format;
    }
};

std::shared_ptr<IDynamicMesh> GL3Renderer::CreateDynamicMesh()
{

}

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window)
{
    return std::shared_ptr<GL3Renderer>(new GL3Renderer(windowManager, window));
}

} // end namespace ng
