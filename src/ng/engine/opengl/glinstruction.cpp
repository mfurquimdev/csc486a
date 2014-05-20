#include "ng/engine/opengl/glinstruction.hpp"

#include "ng/engine/opengl/globject.hpp"
#include "ng/engine/debug.hpp"

#include <cstring>

namespace ng
{

OpenGLInstructionLinearBuffer::OpenGLInstructionLinearBuffer(std::size_t commandBufferSize)
    : mBuffer(commandBufferSize)
    , mReadHead(0)
    , mWriteHead(0)
{ }

bool OpenGLInstructionLinearBuffer::PushInstruction(const OpenGLInstruction& inst)
{
    std::size_t bytesToWrite = inst.GetByteSize();

    // is there not enough room to write it?
    if (bytesToWrite > mBuffer.size() - mWriteHead)
    {
        // sigh... resize the buffer to fit this instruction.
        std::size_t oldSize = mBuffer.size();
        std::size_t newSize = mWriteHead + bytesToWrite;

        DebugPrintf("Resizing OpenGLInstructionLinearBuffer from %zu to %zu\n", oldSize, newSize);

        // TODO: maybe should only resize up to a certain point and return false if that upper bound is reached?
        mBuffer.resize(newSize);
    }

    // the room is available: write the instruction.
    std::memcpy(&mBuffer[mWriteHead], &inst, bytesToWrite);
    mWriteHead += bytesToWrite;

    return true;
}

bool OpenGLInstructionLinearBuffer::CanPopInstruction() const
{
    // can keep popping until the read catches up with the write
    return mReadHead != mWriteHead;
}

bool OpenGLInstructionLinearBuffer::PopInstruction(OpenGLInstruction& inst)
{
    if (!CanPopInstruction())
    {
        return false;
    }

    // first read the header: OpCode and NumParams
    const std::size_t offsetOfParams = offsetof(OpenGLInstruction, Params);
    std::size_t headerSize = offsetOfParams;
    char* instdata = reinterpret_cast<char*>(&inst);

    // copy the header into the data structure.
    std::memcpy(instdata, &mBuffer[mReadHead], headerSize);
    mReadHead += headerSize;

    // next, read the params
    std::size_t paramsSize = sizeof(OpenGLInstruction().Params[0]) * inst.NumParams;
    std::memcpy(instdata + offsetOfParams, &mBuffer[mReadHead], paramsSize);
    mReadHead += paramsSize;

    return true;
}

void OpenGLInstructionLinearBuffer::Reset()
{
    mReadHead = mWriteHead = 0;
}

OpenGLInstructionRingBuffer::OpenGLInstructionRingBuffer(std::size_t instructionBufferSize)
    : mBuffer(instructionBufferSize)
    , mReadHead(0)
    , mWriteHead(0)
{ }

bool OpenGLInstructionRingBuffer::PushInstruction(const OpenGLInstruction& inst)
{
    std::size_t bytesToWrite = inst.GetByteSize();

    // can the write be done in a single step?
    if (bytesToWrite <= mBuffer.size() - mWriteHead)
    {
        // is the read head in the way? this happens if the reading lags behind the writing
        if (mReadHead > mWriteHead && mReadHead - mWriteHead < bytesToWrite)
        {
            // must reallocate queue to create room
            std::size_t numBytesMissing = bytesToWrite - (mReadHead - mWriteHead);

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
        std::size_t firstHalfSize = mBuffer.size() - mWriteHead;
        std::size_t secondHalfSize = bytesToWrite - firstHalfSize;

        // Is the read head in the way?
        if (mReadHead > mWriteHead)
        {
            // in this case, the read head interrupts the first half.
            // must reallocate queue to create room
            std::size_t numBytesMissing = firstHalfSize - (mReadHead - mWriteHead);

            DebugPrintf("Resizing OpenGLInstructionRingBuffer from %zu to %zu\n", mBuffer.size(), mBuffer.size() + numBytesMissing);

            mBuffer.insert(mBuffer.begin() + mWriteHead, numBytesMissing, 0);
            mReadHead += numBytesMissing;

        }
        else if (mReadHead < secondHalfSize)
        {
            // in this case, the read head interrupts the second half.
            std::size_t numBytesMissing = secondHalfSize - mReadHead;

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

bool OpenGLInstructionRingBuffer::CanPopInstruction() const
{
    // can keep popping until the read catches up with the write
    return mReadHead != mWriteHead;
}

bool OpenGLInstructionRingBuffer::PopInstruction(OpenGLInstruction& inst)
{
    if (!CanPopInstruction())
    {
        return false;
    }

    // first read OpCode and NumParams
    const std::size_t offsetOfParams = offsetof(OpenGLInstruction, Params);
    std::size_t bytesToRead = offsetOfParams;
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
        std::size_t firstHalfSize = mBuffer.size() - mReadHead;
        std::size_t secondHalfSize = bytesToRead - firstHalfSize;
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
        std::size_t firstHalfSize = mBuffer.size() - mReadHead;
        std::size_t secondHalfSize = bytesToRead - firstHalfSize;
        std::memcpy(instdata + offsetOfParams, &mBuffer[mReadHead], firstHalfSize);
        std::memcpy(instdata + offsetOfParams + firstHalfSize, &mBuffer[0], secondHalfSize);
        mReadHead = secondHalfSize;
    }

    return true;
}

ClearOpCodeParams::ClearOpCodeParams(GLbitfield mask)
    : Mask(mask)
{ }

ClearOpCodeParams::ClearOpCodeParams(const OpenGLInstruction& inst)
    : Mask(inst.Params[0])
{ }

SizedOpenGLInstruction<1> ClearOpCodeParams::ToInstruction() const
{
    SizedOpenGLInstruction<1> si(OpenGLOpCode::Clear);
    si.Instruction.Params[0] = Mask;
    return si;
}

GenBufferOpCodeParams::GenBufferOpCodeParams(std::unique_ptr<std::promise<OpenGLBuffer>> bufferPromise,
                      bool autoCleanup)
    : BufferPromise(std::move(bufferPromise))
    , AutoCleanup(autoCleanup)
{ }

GenBufferOpCodeParams::GenBufferOpCodeParams(const OpenGLInstruction& inst, bool autoCleanup)
    : BufferPromise(reinterpret_cast<std::promise<OpenGLBuffer>*>(inst.Params[0]))
    , AutoCleanup(autoCleanup)
{ }

GenBufferOpCodeParams::~GenBufferOpCodeParams()
{
    if (!AutoCleanup)
    {
        BufferPromise.release();
    }
}

SizedOpenGLInstruction<1> GenBufferOpCodeParams::ToInstruction() const
{
    SizedOpenGLInstruction<1> si(OpenGLOpCode::GenBuffer);
    si.Instruction.Params[0] = reinterpret_cast<std::uintptr_t>(BufferPromise.get());
    return si;
}


DeleteBufferOpCodeParams::DeleteBufferOpCodeParams(GLuint buffer)
    : Buffer(buffer)
{ }

DeleteBufferOpCodeParams::DeleteBufferOpCodeParams(const OpenGLInstruction& inst)
    : Buffer(inst.Params[0])
{ }

SizedOpenGLInstruction<1> DeleteBufferOpCodeParams::ToInstruction() const
{
    SizedOpenGLInstruction<1> si(OpenGLOpCode::DeleteBuffer);
    si.Instruction.Params[0] = Buffer;
    return si;
}

BufferDataOpCodeParams::BufferDataOpCodeParams(
        std::unique_ptr<std::shared_future<OpenGLBuffer>> bufferHandle,
        GLenum target,
        GLsizeiptr size,
        std::unique_ptr<std::shared_ptr<const void>> dataHandle,
        GLenum usage,
        bool autoCleanup)
    : BufferHandle(std::move(bufferHandle))
    , Target(target)
    , Size(size)
    , DataHandle(std::move(dataHandle))
    , Usage(usage)
    , AutoCleanup(autoCleanup)
{ }

BufferDataOpCodeParams::BufferDataOpCodeParams(const OpenGLInstruction& inst, bool autoCleanup)
    : BufferHandle(reinterpret_cast<std::shared_future<OpenGLBuffer>*>(inst.Params[0]))
    , Target(inst.Params[1])
    , Size(inst.Params[2])
    , DataHandle(reinterpret_cast<std::shared_ptr<const void>*>(inst.Params[3]))
    , Usage(inst.Params[4])
    , AutoCleanup(autoCleanup)
{ }

BufferDataOpCodeParams::~BufferDataOpCodeParams()
{
    if (!AutoCleanup)
    {
        DataHandle.release();
        BufferHandle.release();
    }
}

SizedOpenGLInstruction<5> BufferDataOpCodeParams::ToInstruction() const
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


SwapBuffersOpCodeParams::SwapBuffersOpCodeParams()
{ }

SizedOpenGLInstruction<0> SwapBuffersOpCodeParams::ToInstruction() const
{
    return SizedOpenGLInstruction<0>(OpenGLOpCode::SwapBuffers);
}

QuitOpCodeParams::QuitOpCodeParams()
{ }

SizedOpenGLInstruction<0> QuitOpCodeParams::ToInstruction() const
{
    return SizedOpenGLInstruction<0>(OpenGLOpCode::Quit);
}

} // end namespace ng
