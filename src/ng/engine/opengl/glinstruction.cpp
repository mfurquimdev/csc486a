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
    std::size_t paramsSize = sizeof(OpenGLInstruction::ParamType) * inst.NumParams;
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
    bytesToRead = sizeof(OpenGLInstruction::ParamType) * inst.NumParams;

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

BufferDataOpCodeParams::BufferDataOpCodeParams(
        std::unique_ptr<std::promise<std::shared_ptr<OpenGLBufferHandle>>> bufferDataPromise,
        std::unique_ptr<std::shared_future<std::shared_ptr<OpenGLBufferHandle>>> bufferHandle,
        GLenum target,
        GLsizeiptr size,
        std::unique_ptr<std::shared_ptr<const void>> dataHandle,
        GLenum usage,
        bool autoCleanup)
    : BufferDataPromise(std::move(bufferDataPromise))
    , BufferHandle(std::move(bufferHandle))
    , Target(target)
    , Size(size)
    , DataHandle(std::move(dataHandle))
    , Usage(usage)
    , AutoCleanup(autoCleanup)
{ }

BufferDataOpCodeParams::BufferDataOpCodeParams(const OpenGLInstruction& inst, bool autoCleanup)
    : BufferDataPromise(reinterpret_cast<std::promise<std::shared_ptr<OpenGLBufferHandle>>*>(inst.Params[0]))
    , BufferHandle(reinterpret_cast<std::shared_future<std::shared_ptr<OpenGLBufferHandle>>*>(inst.Params[1]))
    , Target(inst.Params[2])
    , Size(inst.Params[3])
    , DataHandle(reinterpret_cast<std::shared_ptr<const void>*>(inst.Params[4]))
    , Usage(inst.Params[5])
    , AutoCleanup(autoCleanup)
{ }

BufferDataOpCodeParams::~BufferDataOpCodeParams()
{
    if (!AutoCleanup)
    {
        DataHandle.release();
        BufferHandle.release();
        BufferDataPromise.release();
    }
}

SizedOpenGLInstruction<6> BufferDataOpCodeParams::ToInstruction() const
{
    SizedOpenGLInstruction<6> si(OpenGLOpCode::BufferData);
    OpenGLInstruction& inst = si.Instruction;
    inst.Params[0] = reinterpret_cast<std::uintptr_t>(BufferDataPromise.get());
    inst.Params[1] = reinterpret_cast<std::uintptr_t>(BufferHandle.get());
    inst.Params[2] = Target;
    inst.Params[3] = Size;
    inst.Params[4] = reinterpret_cast<std::uintptr_t>(DataHandle.get());
    inst.Params[5] = Usage;
    return si;
}

GenShaderOpCodeParams::GenShaderOpCodeParams(
        std::unique_ptr<std::promise<std::shared_ptr<OpenGLShaderHandle>>> shaderPromise,
        GLenum shaderType, bool autoCleanup)
    : ShaderPromise(std::move(shaderPromise))
    , ShaderType(shaderType)
    , AutoCleanup(autoCleanup)
{ }

GenShaderOpCodeParams::GenShaderOpCodeParams(const OpenGLInstruction &inst, bool autoCleanup)
    : ShaderPromise(reinterpret_cast<std::promise<std::shared_ptr<OpenGLShaderHandle>>*>(inst.Params[0]))
    , ShaderType(inst.Params[1])
    , AutoCleanup(autoCleanup)
{ }

GenShaderOpCodeParams::~GenShaderOpCodeParams()
{
    if (!AutoCleanup)
    {
        ShaderPromise.release();
    }
}

SizedOpenGLInstruction<2> GenShaderOpCodeParams::ToInstruction() const
{
    SizedOpenGLInstruction<2> si(OpenGLOpCode::GenShader);
    si.Instruction.Params[0] = reinterpret_cast<std::uintptr_t>(ShaderPromise.get());
    si.Instruction.Params[1] = ShaderType;
    return si;
}

CompileShaderOpCodeParams::CompileShaderOpCodeParams(
        std::unique_ptr<std::promise<std::shared_ptr<OpenGLShaderHandle>>> compiledShaderPromise,
        std::unique_ptr<std::shared_future<std::shared_ptr<OpenGLShaderHandle>>> shaderHandle,
        std::unique_ptr<std::shared_ptr<const char>> sourceHandle,
        bool autoCleanup)
    : CompiledShaderPromise(std::move(compiledShaderPromise))
    , ShaderHandle(std::move(shaderHandle))
    , SourceHandle(std::move(sourceHandle))
    , AutoCleanup(autoCleanup)
{ }

CompileShaderOpCodeParams::CompileShaderOpCodeParams(const OpenGLInstruction& inst, bool autoCleanup)
    : CompiledShaderPromise(reinterpret_cast<std::promise<std::shared_ptr<OpenGLShaderHandle>>*>(inst.Params[0]))
    , ShaderHandle(reinterpret_cast<std::shared_future<std::shared_ptr<OpenGLShaderHandle>>*>(inst.Params[1]))
    , SourceHandle(reinterpret_cast<std::shared_ptr<const char>*>(inst.Params[2]))
    , AutoCleanup(autoCleanup)
{ }

CompileShaderOpCodeParams::~CompileShaderOpCodeParams()
{
    if (!AutoCleanup)
    {
        ShaderHandle.release();
        SourceHandle.release();
        CompiledShaderPromise.release();
    }
}

SizedOpenGLInstruction<3> CompileShaderOpCodeParams::ToInstruction() const
{
    SizedOpenGLInstruction<3> si(OpenGLOpCode::CompileShader);
    OpenGLInstruction& inst = si.Instruction;
    inst.Params[0] = reinterpret_cast<std::uintptr_t>(CompiledShaderPromise.get());
    inst.Params[1] = reinterpret_cast<std::uintptr_t>(ShaderHandle.get());
    inst.Params[2] = reinterpret_cast<std::uintptr_t>(SourceHandle.get());
    return si;
}

LinkShaderProgramOpCodeParams::LinkShaderProgramOpCodeParams(
        std::unique_ptr<std::promise<std::shared_ptr<OpenGLShaderProgramHandle>>> linkedProgramPromise,
        std::unique_ptr<std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>>> shaderProgramHandle,
        std::unique_ptr<std::shared_future<std::shared_ptr<OpenGLShaderHandle>>> vertexShaderHandle,
        std::unique_ptr<std::shared_future<std::shared_ptr<OpenGLShaderHandle>>> fragmentShaderHandle,
        bool autoCleanup)
    : LinkedProgramPromise(std::move(linkedProgramPromise))
    , ShaderProgramHandle(std::move(shaderProgramHandle))
    , VertexShaderHandle(std::move(vertexShaderHandle))
    , FragmentShaderHandle(std::move(fragmentShaderHandle))
    , AutoCleanup(autoCleanup)
{ }

LinkShaderProgramOpCodeParams::LinkShaderProgramOpCodeParams(
        const OpenGLInstruction& inst, bool autoCleanup)
    : LinkedProgramPromise(reinterpret_cast<std::promise<std::shared_ptr<OpenGLShaderProgramHandle>>*>(inst.Params[0]))
    , ShaderProgramHandle(reinterpret_cast<std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>>*>(inst.Params[1]))
    , VertexShaderHandle(reinterpret_cast<std::shared_future<std::shared_ptr<OpenGLShaderHandle>>*>(inst.Params[2]))
    , FragmentShaderHandle(reinterpret_cast<std::shared_future<std::shared_ptr<OpenGLShaderHandle>>*>(inst.Params[3]))
    , AutoCleanup(autoCleanup)
{ }

LinkShaderProgramOpCodeParams::~LinkShaderProgramOpCodeParams()
{
    if (!AutoCleanup)
    {
        FragmentShaderHandle.release();
        VertexShaderHandle.release();
        ShaderProgramHandle.release();
        LinkedProgramPromise.release();
    }
}

SizedOpenGLInstruction<4> LinkShaderProgramOpCodeParams::ToInstruction() const
{
    SizedOpenGLInstruction<4> si(OpenGLOpCode::LinkShaderProgram);
    OpenGLInstruction& inst = si.Instruction;
    inst.Params[0] = reinterpret_cast<std::uintptr_t>(LinkedProgramPromise.get());
    inst.Params[1] = reinterpret_cast<std::uintptr_t>(ShaderProgramHandle.get());
    inst.Params[2] = reinterpret_cast<std::uintptr_t>(VertexShaderHandle.get());
    inst.Params[3] = reinterpret_cast<std::uintptr_t>(FragmentShaderHandle.get());
    return si;
}

DrawVertexArrayOpCodeParams::DrawVertexArrayOpCodeParams(
        std::unique_ptr<std::shared_future<std::shared_ptr<OpenGLVertexArrayHandle>>> vertexArrayHandle,
        std::unique_ptr<std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>>> programHandle,
        GLenum mode,
        GLint firstVertexIndex,
        GLsizei vertexCount,
        bool autoCleanup)
    : VertexArrayHandle(std::move(vertexArrayHandle))
    , ProgramHandle(std::move(programHandle))
    , Mode(mode)
    , FirstVertexIndex(firstVertexIndex)
    , VertexCount(vertexCount)
    , AutoCleanup(autoCleanup)
{ }

DrawVertexArrayOpCodeParams::DrawVertexArrayOpCodeParams(
        const OpenGLInstruction& inst, bool autoCleanup)
    : VertexArrayHandle(reinterpret_cast<std::shared_future<std::shared_ptr<OpenGLVertexArrayHandle>>*>(inst.Params[0]))
    , ProgramHandle(reinterpret_cast<std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>>*>(inst.Params[1]))
    , Mode(inst.Params[2])
    , FirstVertexIndex(inst.Params[3])
    , VertexCount(inst.Params[4])
    , AutoCleanup(autoCleanup)
{ }

DrawVertexArrayOpCodeParams::~DrawVertexArrayOpCodeParams()
{
    if (!AutoCleanup)
    {
        ProgramHandle.release();
        VertexArrayHandle.release();
    }
}

SizedOpenGLInstruction<5> DrawVertexArrayOpCodeParams::ToInstruction() const
{
    SizedOpenGLInstruction<5> si(OpenGLOpCode::DrawVertexArray);
    OpenGLInstruction& inst(si.Instruction);
    inst.Params[0] = reinterpret_cast<std::uintptr_t>(VertexArrayHandle.get());
    inst.Params[1] = reinterpret_cast<std::uintptr_t>(ProgramHandle.get());
    inst.Params[2] = Mode;
    inst.Params[3] = FirstVertexIndex;
    inst.Params[4] = VertexCount;
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
