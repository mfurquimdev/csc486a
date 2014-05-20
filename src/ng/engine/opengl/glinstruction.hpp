#ifndef NG_GLINSTRUCTION_HPP
#define NG_GLINSTRUCTION_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <future>

#include <GL/gl.h>

namespace ng
{

struct OpenGLInstruction
{
    static const std::size_t MaxParams = 16;

    std::uint32_t OpCode;
    std::size_t NumParams;
    std::uintptr_t Params[1]; // note: flexible array sized by NumParams

    std::size_t GetByteSize() const
    {
        return GetSizeForNumParams(NumParams);
    }

    static std::size_t GetMaxByteSize()
    {
        return GetSizeForNumParams(MaxParams);
    }

    static constexpr std::size_t GetSizeForNumParams(std::size_t params)
    {
        return sizeof(OpenGLInstruction) + (params - 1) * sizeof(Params[0]);
    }
};

static_assert(std::is_pod<OpenGLInstruction>::value, "GPUInstructions must be plain old data.");

class OpenGLInstructionLinearBuffer
{
    std::vector<char> mBuffer;
    std::size_t mReadHead;
    std::size_t mWriteHead;

public:
    OpenGLInstructionLinearBuffer(std::size_t commandBufferSize);

    // Returns true on success, false on failure.
    // On failure, the instruction is not pushed. Failure happens due to not enough memory.
    // Failure can be handled either by increasing commandBufferSize, or by flushing the instructions.
    bool PushInstruction(const OpenGLInstruction& inst);

    // Checks if there is an instruction available to read.
    bool CanPopInstruction() const;

    // returns true and writes to inst if there was something to pop that was popped.
    // returns false otherwise.
    // Warning: make sure you're writing to an instruction allocated with enough room!
    //          recommend allocating a SizedOpenGLInstruction<OpenGLInstruction::MaxParams>
    // nice pattern: while (PopInstruction(inst))
    bool PopInstruction(OpenGLInstruction& inst);

    // Resets the linear buffer back to the start.
    void Reset();
};

class OpenGLInstructionRingBuffer
{
    std::vector<char> mBuffer;
    std::size_t mReadHead;
    std::size_t mWriteHead;

public:
    OpenGLInstructionRingBuffer(std::size_t instructionBufferSize);

    // Returns true on success, false on failure.
    // On failure, the instruction is not pushed. Failure happens due to not enough memory.
    // Failure can be handled either by increasing commandBufferSize, or by flushing the instructions.
    bool PushInstruction(const OpenGLInstruction& inst);

    // returns true if there is an instruction available to read
    bool CanPopInstruction() const;

    // returns true and writes to inst if there was something to pop that was popped.
    // returns false otherwise.
    // Warning: make sure you're writing to an instruction allocated with enough room!
    //          recommend allocating a SizedOpenGLInstruction<OpenGLInstruction::MaxParams>
    // nice pattern: while (PopInstruction(inst))
    bool PopInstruction(OpenGLInstruction& inst);
};

struct OpenGLOpCode
{
    enum OpCodeID
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

    OpCodeID OpCode;

    OpenGLOpCode(OpCodeID codeID)
        : OpCode(codeID)
    { }

    static constexpr const char* CodeIDToString(OpCodeID code)
    {
        return code == Clear ? "Clear"
             : code == GenBuffer ? "GenBuffer"
             : code == DeleteBuffer ? "DeleteBuffer"
             : code == BufferData ? "BufferData"
             : code == SwapBuffers ? "SwapBuffers"
             : code == Quit ? "Quit"
             : "???";
    }

    const char* ToString() const
    {
        return CodeIDToString(OpCode);
    }
};

template<std::size_t NParams>
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
        Instruction.OpCode = static_cast<decltype(Instruction.OpCode)>(code.OpCode);
    }
};

class OpenGLBuffer;
class ResourceHandle;

struct ClearOpCodeParams
{
    GLbitfield Mask;

    ClearOpCodeParams(GLbitfield mask);
    ClearOpCodeParams(const OpenGLInstruction& inst);

    SizedOpenGLInstruction<1> ToInstruction() const;
};

struct GenBufferOpCodeParams
{
    std::unique_ptr<std::promise<OpenGLBuffer>> BufferPromise;

    bool AutoCleanup;

    GenBufferOpCodeParams(std::unique_ptr<std::promise<OpenGLBuffer>> bufferPromise, bool autoCleanup);
    GenBufferOpCodeParams(const OpenGLInstruction& inst, bool autoCleanup);
    ~GenBufferOpCodeParams();

    SizedOpenGLInstruction<1> ToInstruction() const;
};

struct DeleteBufferOpCodeParams
{
    GLuint Buffer;

    DeleteBufferOpCodeParams(GLuint buffer);
    DeleteBufferOpCodeParams(const OpenGLInstruction& inst);

    SizedOpenGLInstruction<1> ToInstruction() const;
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

    BufferDataOpCodeParams(
            std::unique_ptr<ResourceHandle> bufferHandle,
            GLenum target,
            GLsizeiptr size,
            std::unique_ptr<std::shared_ptr<const void>> dataHandle,
            GLenum usage,
            bool autoCleanup);

    BufferDataOpCodeParams(const OpenGLInstruction& inst, bool autoCleanup);

    ~BufferDataOpCodeParams();

    SizedOpenGLInstruction<5> ToInstruction() const;
};

struct SwapBuffersOpCodeParams
{
    SwapBuffersOpCodeParams();

    SizedOpenGLInstruction<0> ToInstruction() const;
};

struct QuitOpCodeParams
{
    QuitOpCodeParams();

    SizedOpenGLInstruction<0> ToInstruction() const;
};

} // end namespace ng

#endif // NG_GLINSTRUCTION_HPP
