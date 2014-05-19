#include "ng/engine/renderer.hpp"

#include "ng/engine/windowmanager.hpp"
#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"
#include "ng/engine/profiler.hpp"
#include "ng/engine/debug.hpp"
#include "ng/engine/vertexformat.hpp"
#include "ng/engine/memory.hpp"
#include "ng/engine/staticmesh.hpp"
#include "ng/engine/semaphore.hpp"
#include "ng/engine/resource.hpp"

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

class OpenGLRenderer;

class OpenGLBuffer
{
    std::shared_ptr<OpenGLRenderer> mRenderer;
    GLuint mHandle;

public:
    OpenGLBuffer();

    OpenGLBuffer(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle);
    ~OpenGLBuffer();

    OpenGLBuffer(const OpenGLBuffer& other) = delete;
    OpenGLBuffer& operator=(const OpenGLBuffer& other) = delete;

    OpenGLBuffer(OpenGLBuffer&& other);
    OpenGLBuffer& operator=(OpenGLBuffer&& other);

    void reset(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle);

    void swap(OpenGLBuffer& other);

    GLuint GetHandle() const
    {
        return mHandle;
    }
};

void swap(OpenGLBuffer& a, OpenGLBuffer& b)
{
    a.swap(b);
}

class OpenGLStaticMesh : public IStaticMesh
{
public:
    std::shared_ptr<OpenGLRenderer> mRenderer;
    VertexFormat mVertexFormat;

    ResourceHandle mVertexBuffer;
    ResourceHandle mIndexBuffer;

    OpenGLStaticMesh(std::shared_ptr<OpenGLRenderer> renderer);

    void Init(const VertexFormat& format,
              std::shared_ptr<const void> vertexData,
              std::ptrdiff_t vertexDataSize,
              std::shared_ptr<const void> indexData,
              std::ptrdiff_t indexDataSize) override;
};

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

enum class OpenGLOpCode : decltype(OpenGLInstruction().OpCode)
{
    Clear,
        // params:
        //         0) GLbitfield mask
        // notes:
        //         * acts as glClear.
        //         * use struct ClearOpCodeParams for convenience.
    GenBuffer,
        // params:
        //         0) std::promise<OpenGLBuffer>* bufferPromise
        // notes:
        //         * the promised OpenGLBuffer must be created by glGenBuffers.
        //         * must then delete bufferPromise.
    DeleteBuffer,
        // params:
        //         0) GLuint buffer
        // notes:
        //         * buffer must be a buffer ID generated by glGenBuffers (or 0.)
        //         * buffer must be deleted with glDeleteBuffers.
    BufferData,
        // params:
        //         0) ng::ResourceHandle* bufferHandle
        //         1) GLenum target
        //         2) GLsizeiptr size
        //         3) std::shared_ptr<const void>* dataHandle
        //         4) GLenum usage
        // notes:
        //         * bufferHandle must wrap a std::shared_future<OpenGLBuffer>.
        //         * must allocate and fill the buffer with data by binding it to target.
        //         * dataHandle may be pointing to shared_ptr(nullptr), which will leave the buffer uninitialized.
        //         * must release ownership of dataHandle (ie. delete dataHandle.)
        //         * must release ownership of bufferHandle (ie. delete bufferHandle.)
        //         * use struct BufferDataOpCodeParams for convenience.
    SwapBuffers,
        // params:
        //         none.
        // notes:
        //         * special instruction to signal a swap of buffers to the window.
    Quit
        // params:
        //         none.
        // notes:
        //         * special instruction to exit the graphics thread.
};

constexpr const char* ToString(OpenGLOpCode code)
{
    return code == OpenGLOpCode::Clear ? "Clear"
         : code == OpenGLOpCode::GenBuffer ? "GenBuffer"
         : code == OpenGLOpCode::DeleteBuffer ? "DeleteBuffer"
         : code == OpenGLOpCode::BufferData ? "BufferData"
         : code == OpenGLOpCode::SwapBuffers ? "SwapBuffers"
         : code == OpenGLOpCode::Quit ? "Quit"
         : "???";
}

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

    SizedOpenGLInstruction(OpenGLOpCode code)
    {
        Instruction.NumParams = NParams;
        Instruction.OpCode = static_cast<decltype(Instruction.OpCode)>(code);
    }
};

struct ClearOpCodeParams
{
    GLbitfield Mask;

    ClearOpCodeParams(GLbitfield mask)
        : Mask(mask)
    { }

    ClearOpCodeParams(const OpenGLInstruction& inst)
        : Mask(inst.Params[0])
    { }

    SizedOpenGLInstruction<1> ToInstruction() const
    {
        SizedOpenGLInstruction<1> si(OpenGLOpCode::Clear);
        si.Instruction.Params[0] = Mask;
        return si;
    }
};

struct GenBufferOpCodeParams
{
    std::unique_ptr<std::promise<OpenGLBuffer>> BufferPromise;

    bool AutoCleanup;

    GenBufferOpCodeParams(std::unique_ptr<std::promise<OpenGLBuffer>> bufferPromise,
                          bool autoCleanup)
        : BufferPromise(std::move(bufferPromise))
        , AutoCleanup(autoCleanup)
    { }

    GenBufferOpCodeParams(const OpenGLInstruction& inst, bool autoCleanup)
        : BufferPromise(reinterpret_cast<std::promise<OpenGLBuffer>*>(inst.Params[0]))
        , AutoCleanup(autoCleanup)
    { }

    ~GenBufferOpCodeParams()
    {
        if (!AutoCleanup)
        {
            BufferPromise.release();
        }
    }

    SizedOpenGLInstruction<1> ToInstruction() const
    {
        SizedOpenGLInstruction<1> si(OpenGLOpCode::GenBuffer);
        si.Instruction.Params[0] = reinterpret_cast<std::uintptr_t>(BufferPromise.get());
        return si;
    }
};

struct DeleteBufferOpCodeParams
{
    GLuint Buffer;

    DeleteBufferOpCodeParams(GLuint buffer)
        : Buffer(buffer)
    { }

    DeleteBufferOpCodeParams(const OpenGLInstruction& inst)
        : Buffer(inst.Params[0])
    { }

    SizedOpenGLInstruction<1> ToInstruction() const
    {
        SizedOpenGLInstruction<1> si(OpenGLOpCode::DeleteBuffer);
        si.Instruction.Params[0] = Buffer;
        return si;
    }
};

struct BufferDataOpCodeParams
{
    std::unique_ptr<ResourceHandle> BufferHandle;
    std::shared_future<OpenGLBuffer>* BufferFuture;
    GLenum Target;
    GLsizeiptr Size;
    std::unique_ptr<std::shared_ptr<const void>> DataHandle;
    GLenum Usage;

    bool AutoCleanup;

    BufferDataOpCodeParams(std::unique_ptr<ResourceHandle> bufferHandle,
                           GLenum target,
                           GLsizeiptr size,
                           std::unique_ptr<std::shared_ptr<const void>> dataHandle,
                           GLenum usage,
                           bool autoCleanup)
        : BufferHandle(std::move(bufferHandle))
        , BufferFuture(BufferHandle->GetData<std::shared_future<OpenGLBuffer>>())
        , Target(target)
        , Size(size)
        , DataHandle(std::move(dataHandle))
        , Usage(usage)
        , AutoCleanup(autoCleanup)
    { }

    BufferDataOpCodeParams(const OpenGLInstruction& inst, bool autoCleanup)
        : BufferHandle(reinterpret_cast<ResourceHandle*>(inst.Params[0]))
        , BufferFuture(BufferHandle->GetData<std::shared_future<OpenGLBuffer>>())
        , Target(inst.Params[1])
        , Size(inst.Params[2])
        , DataHandle(reinterpret_cast<std::shared_ptr<const void>*>(inst.Params[3]))
        , Usage(inst.Params[4])
        , AutoCleanup(autoCleanup)
    { }

    ~BufferDataOpCodeParams()
    {
        if (!AutoCleanup)
        {
            DataHandle.release();
            BufferHandle.release();
        }
    }

    SizedOpenGLInstruction<5> ToInstruction() const
    {
        SizedOpenGLInstruction<5> si(OpenGLOpCode::BufferData);
        OpenGLInstruction& inst = si.Instruction;
        inst.Params[0] = reinterpret_cast<std::uintptr_t>(BufferHandle.get());
        inst.Params[1] = Target;
        inst.Params[2] = Size;
        inst.Params[3] = reinterpret_cast<std::uintptr_t>(DataHandle.get());
        inst.Params[4] = Usage;
        return si;
    }
};

struct SwapBuffersOpCodeParams
{
    SwapBuffersOpCodeParams() { }

    SizedOpenGLInstruction<0> ToInstruction() const
    {
        return SizedOpenGLInstruction<0>(OpenGLOpCode::SwapBuffers);
    }
};

struct QuitOpCodeParams
{
    QuitOpCodeParams() { }

    SizedOpenGLInstruction<0> ToInstruction() const
    {
        return SizedOpenGLInstruction<0>(OpenGLOpCode::Quit);
    }
};

static void OpenGLRenderingThreadEntry(RenderingOpenGLThreadData* threadData);
static void OpenGLResourceThreadEntry(ResourceOpenGLThreadData* threadData);

class OpenGLRenderer: public IRenderer, public std::enable_shared_from_this<OpenGLRenderer>
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

    // classes of objects created by Renderer
    static constexpr ResourceHandle::ClassIDType GLBufferClassID{ "GLVB" };

    // for generating unique IDs for resources
    ResourceHandle::IDType mCurrentID = 0;
    std::mutex mIDGenLock;

    ResourceHandle::IDType GenerateUID()
    {
        std::lock_guard<std::mutex> lock(mIDGenLock);

        mCurrentID++;

        return mCurrentID;
    }

    OpenGLRenderer(
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

    void PushInstruction(CommonOpenGLThreadData& threadData, const OpenGLInstruction& inst)
    {
        if (&threadData == mRenderingThreadData.get())
        {
            PushRenderingInstruction(inst);
        }
        else
        {
            PushResourceInstruction(inst);
        }
    }

    void SendQuit(CommonOpenGLThreadData& threadData)
    {
        auto si = QuitOpCodeParams().ToInstruction();
        PushInstruction(threadData, si.Instruction);
    }

    void SendSwapBuffers()
    {
        PushRenderingInstruction(SwapBuffersOpCodeParams().ToInstruction().Instruction);
    }

    ResourceHandle SendGenBuffer()
    {
        // grab a new promise so we can fill it up.
        GenBufferOpCodeParams params(ng::make_unique<std::promise<OpenGLBuffer>>(), true);
        ResourceHandle res(GenerateUID(), GLBufferClassID,
                           std::make_shared<std::shared_future<OpenGLBuffer>>(
                               params.BufferPromise->get_future()));

        PushResourceInstruction(params.ToInstruction().Instruction);
        params.AutoCleanup = false;

        return res;
    }

    void SendDeleteBuffer(GLuint buffer)
    {
        DeleteBufferOpCodeParams params(buffer);
        PushResourceInstruction(params.ToInstruction().Instruction);
    }

    void SendBufferData(CommonOpenGLThreadData& threadData,
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
        PushInstruction(threadData, params.ToInstruction().Instruction);
        params.AutoCleanup = false;
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
    }

    ~OpenGLRenderer()
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
        ClearOpCodeParams params(  (color   ? GL_COLOR_BUFFER_BIT   : 0)
                                 | (depth   ? GL_DEPTH_BUFFER_BIT   : 0)
                                 | (stencil ? GL_STENCIL_BUFFER_BIT : 0));

        PushRenderingInstruction(params.ToInstruction().Instruction);
    }

    void SwapBuffers() override
    {
        // make sure the buffer we're sending the swapbuffers commmand to
        // is the same that we will switch away from when swapping command queues
        std::lock_guard<std::recursive_mutex> lock(mRenderingThreadData->mCurrentWriteBufferMutex);

        SendSwapBuffers();
        SwapRenderingInstructionQueues();
    }

    std::shared_ptr<IStaticMesh> CreateStaticMesh() override
    {
        return std::make_shared<OpenGLStaticMesh>(shared_from_this());
    }
};

constexpr ResourceHandle::ClassIDType OpenGLRenderer::GLBufferClassID;

OpenGLBuffer::OpenGLBuffer()
    : mHandle(0)
{ }

OpenGLBuffer::OpenGLBuffer(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle)
    : mRenderer(std::move(renderer))
    , mHandle(handle)
{ }

OpenGLBuffer::~OpenGLBuffer()
{
    if (mRenderer && mHandle)
    {
        mRenderer->SendDeleteBuffer(mHandle);
    }
}

OpenGLBuffer::OpenGLBuffer(OpenGLBuffer&& other)
    : OpenGLBuffer()
{
    swap(other);
}

OpenGLBuffer& OpenGLBuffer::operator=(OpenGLBuffer&& other)
{
    swap(other);
    return *this;
}

void OpenGLBuffer::reset(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle)
{
    mRenderer = renderer;
    mHandle = handle;
}

void OpenGLBuffer::swap(OpenGLBuffer& other)
{
    using std::swap;
    swap(mRenderer, other.mRenderer);
    swap(mHandle, other.mHandle);
}

OpenGLStaticMesh::OpenGLStaticMesh(std::shared_ptr<OpenGLRenderer> renderer)
    : mRenderer(std::move(renderer))
{ }

void OpenGLStaticMesh::Init(const VertexFormat& format,
          std::shared_ptr<const void> vertexData,
          std::ptrdiff_t vertexDataSize,
          std::shared_ptr<const void> indexData,
          std::ptrdiff_t indexDataSize)
{
    mVertexFormat = format;

    // upload vertexData
    mVertexBuffer = mRenderer->SendGenBuffer();
    mRenderer->SendBufferData(*mRenderer->mResourceThreadData, mVertexBuffer, GL_ARRAY_BUFFER, vertexDataSize, vertexData, GL_DYNAMIC_DRAW);

    if (indexData != nullptr && !mVertexFormat.IsIndexed)
    {
        throw std::logic_error("Don't send indexData if your vertex format isn't indexed.");
    }

    // upload indexData
    if (mVertexFormat.IsIndexed)
    {
        mIndexBuffer = mRenderer->SendGenBuffer();
        mRenderer->SendBufferData(*mRenderer->mResourceThreadData, mIndexBuffer, GL_ELEMENT_ARRAY_BUFFER, indexDataSize, indexData, GL_STATIC_DRAW);
    }
}

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window)
{
    return std::shared_ptr<OpenGLRenderer>(new OpenGLRenderer(windowManager, window));
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
    switch (static_cast<OpenGLOpCode>(inst.OpCode))
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
            RenderDebugPrintf("Rendering thread processing %s\n", ToString(static_cast<OpenGLOpCode>(inst.OpCode)));
            switch (static_cast<OpenGLOpCode>(inst.OpCode))
            {
            case OpenGLOpCode::Quit: {
                RenderProfilePrintf("Time spent rendering serverside in %s: %lfms\n", threadData->mThreadName.c_str(), renderProfiler.GetTotalTimeMS());
                RenderProfilePrintf("Average time spent rendering serverside in %s: %lfms\n", threadData->mThreadName.c_str(), renderProfiler.GetAverageTimeMS());
                return;
            } break;
            default: {
                HandleCommonInstruction(threadData, inst);
                break;
            }
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

        RenderDebugPrintf("Resource thread processing %s\n", ToString(static_cast<OpenGLOpCode>(inst.OpCode)));

        // now execute the instruction
        switch (static_cast<OpenGLOpCode>(inst.OpCode))
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
            break;
        }
        }

        resourceProfiler.Stop();
    }
}

// clean up defines
#undef GetGLExtension
#undef InitGLExtensions

} // end namespace ng
