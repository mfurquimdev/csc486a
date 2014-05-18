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

struct OpenGLInstructionLinearBuffer
{
    std::vector<char> mBuffer;
    size_t mReadHead;
    size_t mWriteHead;

    OpenGLInstructionLinearBuffer(size_t commandBufferSize)
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

        // is there not enough room to write it?
        if (bytesToWrite > mBuffer.size() - mWriteHead)
        {
            // sigh... resize the buffer to fit this instruction.
            size_t oldSize = mBuffer.size();
            size_t newSize = mWriteHead + bytesToWrite;

            DebugPrintf("Resizing OpenGLInstructionLinearBuffer from %zu to %zu\n", oldSize, newSize);

            // TODO: maybe should only resize up to a certain point and return false if that upper bound is reached?
            mBuffer.resize(newSize);
        }

        // the room is available: write the instruction.
        std::memcpy(&mBuffer[mWriteHead], &inst, bytesToWrite);
        mWriteHead += bytesToWrite;

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

// TODO: this data structure could probably use simplification.
struct OpenGLInstructionRingBuffer
{
    std::vector<char> mBuffer;
    size_t mReadHead;
    size_t mWriteHead;

    OpenGLInstructionRingBuffer(size_t instructionBufferSize)
        : mBuffer(instructionBufferSize)
        , mReadHead(0)
        , mWriteHead(0)
    { }

    // Returns true on success, false on failure.
    // On failure, the instruction is not pushed. Failure happens due to not enough memory.
    // Failure can be handled either by increasing commandBufferSize, or by flushing the instructions.
    bool PushInstruction(const OpenGLInstruction& inst)
    {
        size_t bytesToWrite = inst.GetByteSize();

        // can the write be done in a single step?
        if (bytesToWrite <= mBuffer.size() - mWriteHead)
        {
            // is the read head in the way? this happens if the reading lags behind the writing
            if (mReadHead > mWriteHead && mReadHead - mWriteHead < bytesToWrite)
            {
                // must reallocate queue to create room
                size_t numBytesMissing = bytesToWrite - (mReadHead - mWriteHead);

                DebugPrintf("Resizing OpenGLInstructionRingBuffer from %zu to %zu\n", mBuffer.size(), mBuffer.size() + numBytesMissing);

                mBuffer.insert(mBuffer.begin() + mWriteHead, numBytesMissing, 0);
                mReadHead = mReadHead + numBytesMissing;
            }

            // can now write it in a simple single step
            std::memcpy(&mBuffer[mWriteHead], &inst, bytesToWrite);
            mWriteHead += bytesToWrite;
        }
        else
        {
            // otherwise, do the write in two steps.
            size_t firstHalfSize = mBuffer.size() - mWriteHead;
            size_t secondHalfSize = bytesToWrite - firstHalfSize;

            // Is the read head in the way?
            if (mReadHead > mWriteHead)
            {
                // in this case, the read head interrupts the first half.
                // must reallocate queue to create room
                size_t numBytesMissing = firstHalfSize - (mReadHead - mWriteHead);

                DebugPrintf("Resizing OpenGLInstructionRingBuffer from %zu to %zu\n", mBuffer.size(), mBuffer.size() + numBytesMissing);

                mBuffer.insert(mBuffer.begin() + mWriteHead, numBytesMissing, 0);
                mReadHead += numBytesMissing;

            }
            else if (mReadHead < secondHalfSize)
            {
                // in this case, the read head interrupts the second half.
                size_t numBytesMissing = secondHalfSize - mReadHead;

                DebugPrintf("Resizing OpenGLInstructionRingBuffer from %zu to %zu\n", mBuffer.size(), mBuffer.size() + numBytesMissing);

                mBuffer.insert(mBuffer.begin(), numBytesMissing, 0);
                mReadHead += numBytesMissing;
                mWriteHead += numBytesMissing;
            }

            // finally, can write the first half.
            std::memcpy(&mBuffer[mWriteHead], &inst, firstHalfSize);
            // next, write the second half
            std::memcpy(&mBuffer[0], &inst, secondHalfSize);
            mWriteHead = secondHalfSize;
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

        // first read OpCode and NumParams
        const size_t offsetOfParams = offsetof(OpenGLInstruction, Params);
        size_t bytesToRead = offsetOfParams;
        char* instdata = reinterpret_cast<char*>(&inst);

        // can it be done in one read?
        if (mBuffer.size() - mReadHead >= bytesToRead)
        {
            // then do it in one read
            std::memcpy(instdata, &mBuffer[mReadHead], bytesToRead);
            mReadHead += bytesToRead;
        }
        else
        {
            // otherwise do it in two reads
            size_t firstHalfSize = mBuffer.size() - mReadHead;
            size_t secondHalfSize = bytesToRead - firstHalfSize;
            std::memcpy(instdata, &mBuffer[mReadHead], firstHalfSize);
            std::memcpy(instdata + firstHalfSize, &mBuffer[0], secondHalfSize);
            mReadHead = secondHalfSize;
        }

        // now that we know NumParams, we can read the params too.
        bytesToRead = sizeof(OpenGLInstruction().Params[0]) * inst.NumParams;

        // can it be done in one read?
        if (mBuffer.size() - mReadHead >= bytesToRead)
        {
            // then do it in one read
            std::memcpy(instdata + offsetOfParams, &mBuffer[mReadHead], bytesToRead);
            mReadHead += bytesToRead;
        }
        else
        {
            // otherwise do it in two reads
            size_t firstHalfSize = mBuffer.size() - mReadHead;
            size_t secondHalfSize = bytesToRead - firstHalfSize;
            std::memcpy(instdata + offsetOfParams, &mBuffer[mReadHead], firstHalfSize);
            std::memcpy(instdata + offsetOfParams + firstHalfSize, &mBuffer[0], secondHalfSize);
            mReadHead = secondHalfSize;
        }

        return true;
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
            GL3Renderer& renderer,
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

static void OpenGLRenderingThreadEntry(RenderingOpenGLThreadData* threadData);
static void OpenGLResourceThreadEntry(ResourceOpenGLThreadData* threadData);

class GL3Renderer: public IRenderer, public std::enable_shared_from_this<GL3Renderer>
{
public:
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mRenderingContext;
    std::shared_ptr<IGLContext> mResourceContext;

    std::unique_ptr<RenderingOpenGLThreadData> mRenderingThreadData;
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
        mRenderingThreadData.reset(new RenderingOpenGLThreadData(
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

    // warning: assumes mCurrentWriteBufferMutex is locked by the caller.
    void PushRenderingInstruction(const OpenGLInstruction& inst)
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

    void PushResourceInstruction(const OpenGLInstruction& inst)
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

    void SendQuit(CommonOpenGLThreadData& threadData)
    {
        SizedOpenGLInstruction<0> sizedInst(OpenGLOpCode::Quit);

        if (&threadData == mRenderingThreadData.get())
        {
            PushRenderingInstruction(sizedInst.Instruction);
        }
        else
        {
            PushResourceInstruction(sizedInst.Instruction);
        }
    }

    void SendSwapBuffers()
    {
        SizedOpenGLInstruction<1> sizedInst(OpenGLOpCode::SwapBuffers);
        PushRenderingInstruction(sizedInst.Instruction);
    }

    std::future<std::shared_ptr<GL3Buffers>> SendGenBuffers(GLsizei n)
    {
        SizedOpenGLInstruction<2> sizedInst(OpenGLOpCode::GenBuffers);
        sizedInst.Instruction.Params[0] = n;

        // grab a new promise so we can fill it up.
        auto promisePtr = ng::make_unique<std::promise<std::shared_ptr<GL3Buffers>>>();
        auto fut = promisePtr->get_future();
        sizedInst.Instruction.Params[1] = (std::uintptr_t) promisePtr.get();

        PushResourceInstruction(sizedInst.Instruction);

        promisePtr.release();
        return fut;
    }

    // does not take ownership of buffers. copies out of it.
    void SendDeleteBuffers(size_t n, const GLuint* buffers)
    {
        SizedOpenGLInstruction<2> sizedInst(OpenGLOpCode::DeleteBuffers);
        sizedInst.Instruction.Params[0] = n;

        // grab a new vector so we can fill it up
        std::unique_ptr<GLuint[]> buffersList(new GLuint[n]);
        std::copy(buffers, buffers + n, buffersList.get());
        sizedInst.Instruction.Params[1] = (std::uintptr_t) buffersList.get();

        PushResourceInstruction(sizedInst.Instruction);
        buffersList.release();
    }

    void SwapRenderingInstructionQueues()
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

        RenderDebugPrintf("SwapRenderingInstructionQueues set mCurrentWriteBufferIndex to %zu\n", mRenderingThreadData->mCurrentWriteBufferIndex);
    }

    ~GL3Renderer()
    {
        // add a Quit instruction to the resource instructions then leave it to be executed.
        SendQuit(*mResourceThreadData);

        // add a Quit instruction to the current buffer then swap away from it and leave it to be executed.
        std::unique_lock<std::recursive_mutex> renderingWriteLock(mRenderingThreadData->mCurrentWriteBufferMutex);
        auto renderingWrittenBufferIndex = mRenderingThreadData->mCurrentWriteBufferIndex;
        SendQuit(*mRenderingThreadData);
        SwapRenderingInstructionQueues();
        mRenderingThreadData->mInstructionConsumerMutex[renderingWrittenBufferIndex].unlock();
        renderingWriteLock.unlock();

        // finally join everything, which waits for both their Quit commands to be run.
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
        PushRenderingInstruction(sizedInst.Instruction);

        RenderDebugPrintf("End GL3Renderer::Clear()\n");
    }

    void SwapBuffers() override
    {
        RenderDebugPrintf("Begin GL3Renderer::SwapBuffers()\n");

        // make sure the buffer we're sending the swapbuffers commmand to
        // is the same that we will switch away from when swapping command queues
        std::lock_guard<std::recursive_mutex> lock(mRenderingThreadData->mCurrentWriteBufferMutex);

        SendSwapBuffers();
        SwapRenderingInstructionQueues();

        RenderDebugPrintf("End GL3Renderer::SwapBuffers()\n");
    }

    std::shared_ptr<IDynamicMesh> CreateDynamicMesh() override
    {
        return std::make_shared<GL3DynamicMesh>(shared_from_this());
    }
};

GL3DynamicMesh::GL3DynamicMesh(std::shared_ptr<GL3Renderer> renderer)
    : mRenderer(std::move(renderer))
{
    mFutureBuffers = mRenderer->SendGenBuffers(2);
}

GL3DynamicMesh::~GL3DynamicMesh()
{
    // force the future buffers to have been received.
    GetBuffers();
    // now delete them
     mRenderer->SendDeleteBuffers(mBuffers->size(), mBuffers->data());
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

void OpenGLRenderingThreadEntry(RenderingOpenGLThreadData* threadData)
{
    threadData->mWindowManager->SetCurrentContext(threadData->mWindow, threadData->mContext);

    Profiler renderProfiler;

    size_t bufferToConsumeFrom = !RenderingOpenGLThreadData::InitialWriteBufferIndex;

    while (true)
    {
        bufferToConsumeFrom = !bufferToConsumeFrom;

        OpenGLInstructionLinearBuffer& instructionBuffer = threadData->mInstructionBuffers[bufferToConsumeFrom];

        RenderDebugPrintf("%s is waiting for consumer mutex %zu to start rendering.\n", threadData->mThreadName.c_str(), bufferToConsumeFrom);
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
            default:
                RenderDebugPrintf("Invalid OpCode for %s: %u\n", threadData->mThreadName.c_str(), inst.OpCode);
            }
        }

        renderProfiler.Stop();
        RenderDebugPrintf("Finished reading instruction buffer %zu in %s\n", bufferToConsumeFrom, threadData->mThreadName.c_str());
    }
}

void OpenGLResourceThreadEntry(ResourceOpenGLThreadData* threadData)
{
    threadData->mWindowManager->SetCurrentContext(threadData->mWindow, threadData->mContext);

    // load necessary extensions
    auto& ctx = *threadData->mContext;
    GetExtension(ctx, PFNGLGENBUFFERSPROC, glGenBuffers);
    GetExtension(ctx, PFNGLDELETEBUFFERSPROC, glDeleteBuffers);

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

        // now execute the instruction
        switch (static_cast<OpenGLOpCode>(inst.OpCode))
        {
        case OpenGLOpCode::GenBuffers: {
            RenderDebugPrintf("Doing OpenGLOpCode::GenBuffers\n");
            GLsizei n = inst.Params[0];
            auto pro = (std::promise<std::shared_ptr<GL3Buffers>>*) inst.Params[1];
            auto buffers = std::make_shared<GL3Buffers>(n);
            glGenBuffers(n, buffers->data());
            pro->set_value(buffers);
            RenderDebugPrintf("Done OpenGLOpCode::GenBuffers\n");
        } break;
        case OpenGLOpCode::DeleteBuffers: {
            RenderDebugPrintf("Doing OpenGLOpCode::DeleteBuffers\n");
            GLsizei n = inst.Params[0];
            GLuint* buffers = (GLuint*) inst.Params[1];
            glDeleteBuffers(n, buffers);
            delete[] buffers;
            RenderDebugPrintf("Done OpenGLOpCode::DeleteBuffers\n");
        } break;
        case OpenGLOpCode::Quit: {
            RenderProfilePrintf("Time spent loading resources serverside in %s: %lfms\n", threadData->mThreadName.c_str(), resourceProfiler.GetTotalTimeMS());
            RenderProfilePrintf("Average time spent loading resources serverside in %s: %lfms\n", threadData->mThreadName.c_str(), resourceProfiler.GetAverageTimeMS());
            return;
        } break;
        default:
            RenderDebugPrintf("Invalid OpCode for %s: %u\n", threadData->mThreadName.c_str(), inst.OpCode);
        }

        resourceProfiler.Stop();
    }
}

// clean up GetExtension define
#undef GetExtension

} // end namespace ng
