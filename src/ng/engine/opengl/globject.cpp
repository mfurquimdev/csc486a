#include "ng/engine/opengl/globject.hpp"

#include "ng/engine/opengl/glrenderer.hpp"

namespace ng
{

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
    if (this != &other)
    {
        swap(other);
    }
    return *this;
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
    mRenderer->SendBufferData(OpenGLRenderer::ResourceInstructionHandler, mVertexBuffer, GL_ARRAY_BUFFER, vertexDataSize, vertexData, GL_DYNAMIC_DRAW);

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
        mIndexBuffer = nullptr;
    }
}

} // end namespace ng
