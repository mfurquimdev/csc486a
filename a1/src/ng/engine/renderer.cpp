#include "ng/engine/renderer.hpp"

#include "ng/engine/windowmanager.hpp"
#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"

#include <GL/gl.h>

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
    static const int InitialWriteBufferIndex = 0;

    CommonOpenGLThreadData(
            const std::shared_ptr<IWindowManager>& windowManager,
            const std::shared_ptr<IWindow>& window,
            const std::shared_ptr<IGLContext>& context,
            size_t commandBufferSize)
            : mWindowManager(windowManager)
            , mWindow(window)
            , mContext(context)
            , mCommandBuffers{commandBufferSize, commandBufferSize}
            , mCurrentWriteBufferIndex(InitialWriteBufferIndex)
    { }

    std::shared_ptr<IWindowManager> mWindowManager;
    std::shared_ptr<IWindow> mWindow;
    std::shared_ptr<IGLContext> mContext;

    OpenGLInstructionRingBuffer mCommandBuffers[2];
    size_t mCurrentWriteBufferIndex;

    std::mutex mBufferSwapMutex;
    std::condition_variable mBufferSwapCondition;
};

enum class OpenGLOpCode : decltype(OpenGLInstruction().OpCode)
{
    Clear, // params: same as glClear.
    SwapBuffers, // special instruction to signal a buffer swap.
    Quit   // special instruction to stop the graphics thread.
};

struct NoInitTag { };

template<size_t NParams>
struct SizedOpenGLInstruction
{
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

    size_t CurrentReadBufferIndex = !CommonOpenGLThreadData::InitialWriteBufferIndex;

    while (true)
    {
        {
            // wait for the write buffer to be done writing, and for it to want to start writing to the read buffer
            std::unique_lock<std::mutex> lock(threadData->mBufferSwapMutex);
            threadData->mBufferSwapCondition.wait(lock, [&]{
                return CurrentReadBufferIndex == threadData->mCurrentWriteBufferIndex;
            });
        }

        // switch away from the write buffer to the read buffer
        CurrentReadBufferIndex = !CurrentReadBufferIndex;

        SizedOpenGLInstruction<OpenGLInstruction::MaxParams> sizedInst(NoInitTag{});
        OpenGLInstruction& inst = sizedInst.Instruction;

        OpenGLInstructionRingBuffer& instructionBuffer = threadData->mCommandBuffers[CurrentReadBufferIndex];
        while (instructionBuffer.PopInstruction(inst))
        {
            switch (static_cast<OpenGLOpCode>(inst.OpCode))
            {
            case OpenGLOpCode::Clear: {
                GLbitfield mask = inst.Params[0];
                glClear(mask);
            } break;
            case OpenGLOpCode::SwapBuffers: {
                threadData->mWindow->SwapBuffers();
            } break;
            case OpenGLOpCode::Quit: {
                return;
            } break;
            }
        }
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
        , mRenderingThreadData(windowManager,
                                     window,
                                     mRenderingContext,
                                     RenderingCommandBufferSize)
        , mResourceThreadData(windowManager,
                                     window,
                                     mResourceContext,
                                     ResourceCommandBufferSize)
        , mRenderingThread(OpenGLThreadEntry, &mRenderingThreadData)
        , mResourceThread(OpenGLThreadEntry, &mResourceThreadData)
    { }

    void PushInstruction(CommonOpenGLThreadData& threadData, const OpenGLInstruction& inst)
    {
        threadData.mCommandBuffers[threadData.mCurrentWriteBufferIndex].PushInstruction(inst);
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

    void SwapCommandQueues(CommonOpenGLThreadData& threadData)
    {
        threadData.mCurrentWriteBufferIndex = !threadData.mCurrentWriteBufferIndex;
    }

    ~GL3Renderer()
    {
        SendQuit(mResourceThreadData);
        SwapCommandQueues(mResourceThreadData);
        mResourceThreadData.mBufferSwapCondition.notify_one();

        SendQuit(mRenderingThreadData);
        SwapCommandQueues(mRenderingThreadData);
        mRenderingThreadData.mBufferSwapCondition.notify_one();

        mResourceThread.join();
        mRenderingThread.join();
    }

    void Clear(bool color, bool depth, bool stencil) override
    {
        SizedOpenGLInstruction<1> sizedInst(OpenGLOpCode::Clear);
        sizedInst.Instruction.Params[0] = (color   ? GL_COLOR_BUFFER_BIT   : 0)
                                        | (depth   ? GL_DEPTH_BUFFER_BIT   : 0)
                                        | (stencil ? GL_STENCIL_BUFFER_BIT : 0);
        PushInstruction(mRenderingThreadData, sizedInst.Instruction);
    }

    void SwapBuffers() override
    {
        SendSwapBuffers(mRenderingThreadData);
        SwapCommandQueues(mRenderingThreadData);
        mRenderingThreadData.mBufferSwapCondition.notify_one();
    }
};

std::shared_ptr<IRenderer> CreateRenderer(
        std::shared_ptr<IWindowManager> windowManager,
        std::shared_ptr<IWindow> window)
{
    return std::shared_ptr<GL3Renderer>(new GL3Renderer(windowManager, window));
}

} // end namespace ng
