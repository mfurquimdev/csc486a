#include "ng/engine/opengl/globject.hpp"

#include "ng/engine/opengl/glrenderer.hpp"

#include "ng/engine/opengl/glenumconversion.hpp"

namespace ng
{

namespace openglobjectpolicies
{

void BufferPolicy::Release(OpenGLRenderer& renderer, GLuint handle)
{
    if (handle)
    {
        renderer.SendDeleteBuffer(handle);
    }
}

void ShaderPolicy::Release(OpenGLRenderer& renderer, GLuint handle)
{
    if (handle)
    {
        renderer.SendDeleteShader(handle);
    }
}

void VertexArrayPolicy::Release(OpenGLRenderer& renderer, GLuint handle)
{
    if (handle)
    {
        renderer.SendDeleteVertexArray(handle);
    }
}

void VertexArrayPolicy::AddDependents(std::vector<std::shared_ptr<OpenGLBufferHandle>> arrayBuffers,
                                      std::shared_ptr<OpenGLBufferHandle> elementArrayBuffer)
{
    mArrayBuffers = std::move(arrayBuffers);
    mElementArrayBuffer = std::move(elementArrayBuffer);
}

void ShaderProgramPolicy::Release(OpenGLRenderer& renderer, GLuint handle)
{
    if (handle)
    {
        renderer.SendDeleteShaderProgram(handle);
    }
}

void ShaderProgramPolicy::AddDependents(
        std::shared_ptr<OpenGLShaderHandle> vertexShader,
        std::shared_ptr<OpenGLShaderHandle> fragmentShader)
{
    mVertexShader = std::move(vertexShader);
    mFragmentShader = std::move(fragmentShader);
}

} // end namespace detail

OpenGLShaderProgram::OpenGLShaderProgram(std::shared_ptr<OpenGLRenderer> renderer)
    : mRenderer(std::move(renderer))
{ }

void OpenGLShaderProgram::Init(
        std::shared_ptr<const char> vertexShaderSource,
        std::shared_ptr<const char> fragmentShaderSource,
        bool validate)
{
    mVertexShader = mRenderer->SendGenShader(GL_VERTEX_SHADER);
    mFragmentShader = mRenderer->SendGenShader(GL_FRAGMENT_SHADER);
    mProgram = mRenderer->SendGenShaderProgram();

    mVertexShader = mRenderer->SendCompileShader(mVertexShader, vertexShaderSource);
    mFragmentShader = mRenderer->SendCompileShader(mFragmentShader, fragmentShaderSource);
    mProgram = mRenderer->SendLinkProgram(mProgram, mVertexShader, mFragmentShader);

    if (validate)
    {
        std::pair<bool,std::string> status = GetStatus();
        if (!status.first)
        {
            throw std::runtime_error(status.second);
        }
    }
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

OpenGLSharedFuture<std::shared_ptr<OpenGLShaderProgramHandle>> OpenGLShaderProgram::GetFutureHandle() const
{
    return mProgram;
}

VertexArray::VertexArray(
    VertexFormat format,
    OpenGLSharedFuture<std::shared_ptr<OpenGLVertexArrayHandle>> vertexArrayHandle,
    std::map<VertexAttributeName, OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>>> attributeBuffers,
    OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>> indexBuffer,
    std::size_t vertexCount)
    : Format(std::move(format))
    , VertexArrayHandle(std::move(vertexArrayHandle))
    , AttributeBuffers(std::move(attributeBuffers))
    , IndexBuffer(std::move(indexBuffer))
    , VertexCount(vertexCount)
{ }

OpenGLStaticMesh::OpenGLStaticMesh(std::shared_ptr<OpenGLRenderer> renderer)
    : mRenderer(std::move(renderer))
{ }

void OpenGLStaticMesh::Reset()
{
    mVertexArray = VertexArray();
}

void OpenGLStaticMesh::Init(
       VertexFormat format,
       std::map<VertexAttributeName,std::pair<std::shared_ptr<const void>,std::ptrdiff_t>> attributeDataAndSize,
       std::shared_ptr<const void> indexData,
       std::ptrdiff_t indexDataSize,
       std::size_t vertexCount)
{
    // hopefully catch some user errors
    if (format.IsIndexed && indexData == nullptr)
    {
        throw std::logic_error("format.IsIndexed == true, but indexData == nullptr");
    }
    if (!format.IsIndexed && indexData != nullptr)
    {
        throw std::logic_error("format.IsIndexed == false, but indexData != nullptr");
    }

    // to make sure we don't generate the same buffer twice
    std::map<std::pair<std::shared_ptr<const void>,std::ptrdiff_t>, VertexAttributeName> uniqueBuffers;

    // upload vertexData
    std::map<VertexAttributeName,OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>>> vertexBuffers;
    for (const auto& attrib : attributeDataAndSize)
    {
        const std::pair<std::shared_ptr<const void>,std::ptrdiff_t>& dataAndSize = attrib.second;

        auto it = uniqueBuffers.find(dataAndSize);
        if (it != uniqueBuffers.end())
        {
            OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>> buf = vertexBuffers.at(it->second);
            vertexBuffers.emplace(attrib.first, std::move(buf));
        }
        else
        {
            OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>> buf = mRenderer->SendGenBuffer();
            buf = mRenderer->SendBufferData(OpenGLRenderer::ResourceInstructionHandler, buf,
                                            GL_ARRAY_BUFFER, dataAndSize.second, dataAndSize.first, GL_STATIC_DRAW);

            vertexBuffers.emplace(attrib.first, std::move(buf));
        }
    }

    OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>> indexBuffer;

    // upload indexData
    if (format.IsIndexed)
    {
        indexBuffer = mRenderer->SendGenBuffer();
        indexBuffer = mRenderer->SendBufferData(OpenGLRenderer::ResourceInstructionHandler, indexBuffer,
                                  GL_ELEMENT_ARRAY_BUFFER, indexDataSize, indexData, GL_STATIC_DRAW);
    }

    OpenGLSharedFuture<std::shared_ptr<OpenGLVertexArrayHandle>> vao = mRenderer->SendGenVertexArray();
    vao = mRenderer->SendSetVertexArrayLayout(vao, format, vertexBuffers, indexBuffer);

    mVertexArray = VertexArray(std::move(format), std::move(vao),
                               std::move(vertexBuffers), std::move(indexBuffer), vertexCount);
}

void OpenGLStaticMesh::Draw(std::shared_ptr<IShaderProgram> program,
                            std::map<std::string, UniformValue> uniforms,
                            RenderState renderState,
                            PrimitiveType primitiveType, std::size_t firstVertexIndex, std::size_t vertexCount)
{
    if (vertexCount == 0)
        return;

    std::shared_ptr<OpenGLShaderProgram> programGL = std::static_pointer_cast<OpenGLShaderProgram>(program);
    mRenderer->SendDrawVertexArray(mVertexArray.VertexArrayHandle, programGL->GetFutureHandle(),
                                   std::move(uniforms), std::move(renderState),
                                   ToGLPrimitiveType(primitiveType), firstVertexIndex, vertexCount,
                                   mVertexArray.Format.IsIndexed, mVertexArray.Format.IndexType);
}

std::size_t OpenGLStaticMesh::GetVertexCount() const
{
    return mVertexArray.VertexCount;
}

} // end namespace ng
