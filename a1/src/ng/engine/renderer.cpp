#include "ng/engine/renderer.hpp"

#include "ng/engine/windowmanager.hpp"
#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"

#include <thread>
#include <cstdint>
#include <vector>
#include <limits>
#include <cstring>
#include <algorithm>
#include <cstddef>
#include <condition_variable>

namespace ng
{

struct OpenGLInstruction
{
    std::uint32_t OpCode;
    size_t NumParams;
    std::uintptr_t Params[1]; // note: flexible array sized by NumParams

    size_t GetByteSize() const
    {
        return sizeof(OpenGLInstruction) + (NumParams - 1) * sizeof(Params[0]);
    }
};

static_assert(std::is_pod<OpenGLInstruction>::value, "GPUInstructions must be plain old data.");

struct OpenGLInstructionRingBuffer
{
    std::vector<char> mBuffer;
    size_t mReadHead;
    size_t mWriteHead;

    OpenGLInstructionRingBuffer(size_t commandBufferSize)
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

        // can the write be done in a single step?
        if (bytesToWrite <= mBuffer.size() - mWriteHead)
        {
            // is the read head in the way? this happens if the reading lags behind the writing
            if (mReadHead > mWriteHead && mReadHead - mWriteHead < bytesToWrite)
            {
                return false;
            }
            else
            {
                // in this case, the read head is not in the way. easy!
                std::memcpy(&mBuffer[mWriteHead], &inst, bytesToWrite);
                mWriteHead += bytesToWrite;
            }
        }
        else
        {
            // otherwise, do the write in two steps.
            size_t firstHalfSize = mBuffer.size() - mWriteHead;
            size_t secondHalfSize = bytesToWrite - firstHalfSize;

            // Is the read head in the way?
            if (mReadHead > mWriteHead      // in this case, the read head interrupts the first half.
             || mReadHead < secondHalfSize) // in this case, the read head interrupts the second half.
            {
                return false;
            }
            else
            {
                // otherwise, can write the first half.
                std::memcpy(&mBuffer[mWriteHead], &inst, firstHalfSize);
                // next, write the second half
                std::memcpy(&mBuffer[0], &inst, secondHalfSize);
                mWriteHead = secondHalfSize;
            }
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
        // TODO: Implement.
        // Should be able to pop a full instruction as long as CanPopInstruction() is true.
        // Begin by reading the OpCode and NumParams, then use NumParams to know how to read the rest.
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

struct CommonOpenGLThreadData
{
    CommonOpenGLThreadData(
            const std::shared_ptr<IWindowManager>& windowManager,
            const std::shared_ptr<IWindow>& window,
            const std::shared_ptr<IGLContext>& context,
            size_t commandBufferSize)
            : mWindowManager(windowManager)
            , mWindow(window)
            , mContext(context)
            , mCommandBuffers{commandBufferSize, commandBufferSize}
            , mWriteCommandBuffer(0)
    { }

    std::shared_ptr<IWindowManager> mWindowManager;
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mContext;

    OpenGLInstructionRingBuffer mCommandBuffers[2];
    size_t mWriteCommandBuffer;

    std::mutex mBufferSwapMutex;
    std::condition_variable mBufferSwapCondition;
};

static void OpenGLThreadEntry(CommonOpenGLThreadData* threadData)
{
    threadData->mWindowManager->SetCurrentContext(threadData->mWindow, threadData->mContext);

    // TODO: Stare at this really hard
//    int CurrentReadCommandBuffer = !threadData->mWriteCommandBuffer;

//    while (true)
//    {
//        {
//            std::unique_lock<std::mutex> lock(threadData->mBufferSwapMutex);
//            threadData->mBufferSwapCondition.wait(lock, [&]{
//                return CurrentReadCommandBuffer != threadData->mWriteCommandBuffer;
//            });
//        }

//        CurrentReadCommandBuffer = !CurrentReadCommandBuffer;
//    }
}

class GL3Renderer: public IRenderer
{
public:
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mRenderingContext;
    std::shared_ptr<IGLContext> mResourceContext;

    CommonOpenGLThreadData mRenderingThreadCommonData;
    CommonOpenGLThreadData mResourceThreadCommonData;

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
        , mRenderingThreadCommonData(windowManager,
                                     window,
                                     mRenderingContext,
                                     RenderingCommandBufferSize)
        , mResourceThreadCommonData(windowManager,
                                     window,
                                     mResourceContext,
                                     ResourceCommandBufferSize)
        , mRenderingThread(OpenGLThreadEntry, &mRenderingThreadCommonData)
        , mResourceThread(OpenGLThreadEntry, &mResourceThreadCommonData)
    { }

    ~GL3Renderer()
    {
        // TODO: Must give the threads a signal to tell them that they should start shutting down.
        mResourceThread.join();
        mRenderingThread.join();
    }
};

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window)
{
    return std::shared_ptr<GL3Renderer>(new GL3Renderer(windowManager, window));
}

} // end namespace ng
