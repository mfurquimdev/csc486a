#include "ng/engine/opengl/globject.hpp"

#include "ng/engine/opengl/glrenderer.hpp"

namespace ng
{

OpenGLBufferHandle::OpenGLBufferHandle()
    : mHandle(0)
{ }

OpenGLBufferHandle::~OpenGLBufferHandle()
{
    if (mRenderer && mHandle)
    {
        mRenderer->SendDeleteBuffer(mHandle);
    }
}

OpenGLBufferHandle::OpenGLBufferHandle(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle)
    : mRenderer(std::move(renderer))
    , mHandle(handle)
{ }

OpenGLBufferHandle::OpenGLBufferHandle(OpenGLBufferHandle&& other)
    : OpenGLBufferHandle()
{
    swap(other);
}

OpenGLBufferHandle& OpenGLBufferHandle::operator=(OpenGLBufferHandle&& other)
{
    if (this != &other)
    {
        swap(other);
    }
    return *this;
}

void OpenGLBufferHandle::swap(OpenGLBufferHandle& other)
{
    using std::swap;
    swap(mRenderer, other.mRenderer);
    swap(mHandle, other.mHandle);
}

GLuint OpenGLBufferHandle::GetHandle() const
{
    return mHandle;
}

OpenGLShaderHandle::OpenGLShaderHandle()
    : mHandle(0)
{ }

OpenGLShaderHandle::~OpenGLShaderHandle()
{
    if (mRenderer && mHandle)
    {
        mRenderer->SendDeleteShader(mHandle);
    }
}

OpenGLShaderHandle::OpenGLShaderHandle(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle)
    : mRenderer(std::move(renderer))
    , mHandle(handle)
{ }

OpenGLShaderHandle::OpenGLShaderHandle(OpenGLShaderHandle&& other)
    : OpenGLShaderHandle()
{
    swap(other);
}

OpenGLShaderHandle& OpenGLShaderHandle::operator=(OpenGLShaderHandle&& other)
{
    if (this != &other)
    {
        swap(other);
    }
    return *this;
}

void OpenGLShaderHandle::swap(OpenGLShaderHandle& other)
{
    using std::swap;
    swap(mRenderer, other.mRenderer);
    swap(mHandle, other.mHandle);
}

GLuint OpenGLShaderHandle::GetHandle() const
{
    return mHandle;
}

void swap(OpenGLShaderHandle& a, OpenGLShaderHandle& b)
{
    a.swap(b);
}

OpenGLShaderProgramHandle::OpenGLShaderProgramHandle()
    : mHandle(0)
{ }

OpenGLShaderProgramHandle::~OpenGLShaderProgramHandle()
{
    if (mRenderer && mHandle)
    {
        mRenderer->SendDeleteShaderProgram(mHandle);
    }
}

OpenGLShaderProgramHandle::OpenGLShaderProgramHandle(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle)
    : mRenderer(std::move(renderer))
    , mHandle(handle)
{ }

OpenGLShaderProgramHandle::OpenGLShaderProgramHandle(OpenGLShaderProgramHandle&& other)
    : OpenGLShaderProgramHandle()
{
    swap(other);
}

OpenGLShaderProgramHandle& OpenGLShaderProgramHandle::operator=(OpenGLShaderProgramHandle&& other)
{
    if (this != &other)
    {
        swap(other);
    }
    return *this;
}

void OpenGLShaderProgramHandle::swap(OpenGLShaderProgramHandle& other)
{
    using std::swap;
    swap(mRenderer, other.mRenderer);
    swap(mHandle, other.mHandle);
}

GLuint OpenGLShaderProgramHandle::GetHandle() const
{
    return mHandle;
}

void swap(OpenGLShaderProgramHandle& a, OpenGLShaderProgramHandle& b)
{
    a.swap(b);
}

OpenGLShaderProgram::OpenGLShaderProgram(std::shared_ptr<OpenGLRenderer> renderer)
    : mRenderer(std::move(renderer))
{ }

void OpenGLShaderProgram::Init(
        std::shared_ptr<const char> vertexShaderSource,
        std::shared_ptr<const char> fragmentShaderSource)
{
    mVertexShader = mRenderer->SendGenShader();
    mFragmentShader = mRenderer->SendGenShader();
    mProgram = mRenderer->SendGenShaderProgram();

    mRenderer->SendCompileShader(mVertexShader, vertexShaderSource);
    mRenderer->SendCompileShader(mFragmentShader, fragmentShaderSource);
    mRenderer->SendLinkProgram(mProgram, mVertexShader, mFragmentShader);
}

std::pair<bool,std::string> OpenGLShaderProgram::GetStatus() const
{
    std::pair<bool,std::string> status;
    status.first = true;

    auto vertexShaderStatus = mRenderer->SendGetShaderStatus(mVertexShader).get();
    auto fragmentShaderStatus = mRenderer->SendGetShaderStatus(mFragmentShader).get();

    if (!vertexShaderStatus.first)
    {
        status.first = false;
        status.second += vertexShaderStatus.second + "\n";
    }

    if (!fragmentShaderStatus.first)
    {
        status.first = false;
        status.second += fragmentShaderStatus.second + "\n";
    }

    if (status.first)
    {
        auto programStatus = mRenderer->SendGetProgramStatus(mProgram).get();
        if (!programStatus.first)
        {
            status.first = false;
            status.second += programStatus.second + "\n";
        }
    }

    return status;
}

OpenGLStaticMesh::OpenGLStaticMesh(std::shared_ptr<OpenGLRenderer> renderer)
    : mRenderer(std::move(renderer))
{ }

void OpenGLStaticMesh::Init(
        const VertexFormat& format,
        const std::vector<std::pair<std::shared_ptr<const void>,std::ptrdiff_t>>& vertexDataAndSize,
        std::shared_ptr<const void> indexData,
        std::ptrdiff_t indexDataSize)
{
    mVertexFormat = format;

    // upload vertexData
    mVertexBuffers.clear();
    for (const auto& dataAndSize : vertexDataAndSize)
    {
        mVertexBuffers.push_back(mRenderer->SendGenBuffer());
        mRenderer->SendBufferData(OpenGLRenderer::ResourceInstructionHandler, mVertexBuffers.back(),
                                  GL_ARRAY_BUFFER, dataAndSize.second, dataAndSize.first, GL_STATIC_DRAW);
    }

    if (indexData != nullptr && !mVertexFormat.IsIndexed)
    {
        throw std::logic_error("Don't send indexData if your vertex format isn't indexed.");
    }

    // upload indexData
    if (mVertexFormat.IsIndexed)
    {
        mIndexBuffer = mRenderer->SendGenBuffer();
        mRenderer->SendBufferData(OpenGLRenderer::ResourceInstructionHandler, mIndexBuffer, GL_ELEMENT_ARRAY_BUFFER, indexDataSize, indexData, GL_STATIC_DRAW);
    }
    else
    {
        mIndexBuffer = std::shared_future<OpenGLBufferHandle>();
    }
}

} // end namespace ng
