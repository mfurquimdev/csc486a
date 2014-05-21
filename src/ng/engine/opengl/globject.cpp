#include "ng/engine/opengl/globject.hpp"

#include "ng/engine/opengl/glrenderer.hpp"

#include "ng/engine/opengl/glenumconversion.hpp"

namespace ng
{

namespace detail
{

void BufferReleasePolicy::Release(std::shared_ptr<OpenGLRenderer>& renderer, GLuint handle)
{
    if (renderer && handle)
    {
        renderer->SendDeleteBuffer(handle);
    }
}

void VertexArrayReleasePolicy::Release(std::shared_ptr<OpenGLRenderer>& renderer, GLuint handle)
{
    if (renderer && handle)
    {
        renderer->SendDeleteVertexArray(handle);
    }
}

void ShaderReleasePolicy::Release(std::shared_ptr<OpenGLRenderer>& renderer, GLuint handle)
{
    if (renderer && handle)
    {
        renderer->SendDeleteShader(handle);
    }
}

void ShaderProgramReleasePolicy::Release(std::shared_ptr<OpenGLRenderer>& renderer, GLuint handle)
{
    if (renderer && handle)
    {
        renderer->SendDeleteShaderProgram(handle);
    }
}

} // end namespace detail

//VertexArray::VertexArray(
//        const VertexFormat& format,
//        std::vector<std::shared_future<std::shared_ptr<OpenGLBufferHandle>>> vertexBufferHandles,
//        std::shared_future<std::shared_ptr<OpenGLBufferHandle>> indexBufferHandle,
//        std::size_t vertexCount)
//    : Format(format)
//    , VertexCount(vertexCount)
//{
//    if (Format.Position.IsEnabled)
//    {
//        Position = std::move(vertexBufferHandles.at(Format.Position.Index));
//    }

//    if (Format.Texcoord0.IsEnabled)
//    {
//        Texcoord0 = std::move(vertexBufferHandles.at(Format.Texcoord0.Index));
//    }

//    if (Format.Texcoord1.IsEnabled)
//    {
//        Texcoord1 = std::move(vertexBufferHandles.at(Format.Texcoord1.Index));
//    }

//    if (Format.Normal.IsEnabled)
//    {
//        Normal = std::move(vertexBufferHandles.at(Format.Normal.Index));
//    }

//    Indices = std::move(indexBufferHandle);
//}

OpenGLShaderProgram::OpenGLShaderProgram(std::shared_ptr<OpenGLRenderer> renderer)
    : mRenderer(std::move(renderer))
{ }

void OpenGLShaderProgram::Init(
        std::shared_ptr<const char> vertexShaderSource,
        std::shared_ptr<const char> fragmentShaderSource)
{
    mVertexShader = mRenderer->SendGenShader(GL_VERTEX_SHADER);
    mFragmentShader = mRenderer->SendGenShader(GL_FRAGMENT_SHADER);
    mProgram = mRenderer->SendGenShaderProgram();

    mVertexShader = mRenderer->SendCompileShader(mVertexShader, vertexShaderSource);
    mFragmentShader = mRenderer->SendCompileShader(mFragmentShader, fragmentShaderSource);
    mProgram = mRenderer->SendLinkProgram(mProgram, mVertexShader, mFragmentShader);
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

std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>> OpenGLShaderProgram::GetFutureHandle() const
{
    return mProgram;
}

OpenGLStaticMesh::OpenGLStaticMesh(std::shared_ptr<OpenGLRenderer> renderer)
    : mRenderer(std::move(renderer))
{ }

void OpenGLStaticMesh::Init(
       VertexFormat format,
       std::map<VertexAttributeName,std::pair<std::shared_ptr<const void>,std::ptrdiff_t>> attributeDataAndSize,
       std::shared_ptr<const void> indexData,
       std::ptrdiff_t indexDataSize,
       std::size_t vertexCount)
{
    // upload vertexData
    std::map<VertexAttributeName,std::shared_future<std::shared_ptr<OpenGLBufferHandle>>> vertexBuffers;
    for (const auto& attrib : attributeDataAndSize)
    {
        const std::pair<std::shared_ptr<const void>,std::ptrdiff_t>& dataAndSize = attrib.second;

        std::shared_future<std::shared_ptr<OpenGLBufferHandle>> buf = mRenderer->SendGenBuffer();
        buf = mRenderer->SendBufferData(OpenGLRenderer::ResourceInstructionHandler, buf,
                                        GL_ARRAY_BUFFER, dataAndSize.second, dataAndSize.first, GL_STATIC_DRAW);

        vertexBuffers.emplace(attrib.first, std::move(buf));
    }

    std::shared_future<std::shared_ptr<OpenGLBufferHandle>> indexBuffer;

    // upload indexData
    if (format.IsIndexed)
    {
        indexBuffer = mRenderer->SendGenBuffer();
        indexBuffer = mRenderer->SendBufferData(OpenGLRenderer::ResourceInstructionHandler, indexBuffer,
                                  GL_ELEMENT_ARRAY_BUFFER, indexDataSize, indexData, GL_STATIC_DRAW);
    }

    std::shared_future<std::shared_ptr<OpenGLVertexArrayHandle>> vao = mRenderer->SendGenVertexArray();
    // TODO: Call a function that automatically sends down all the necessary commands to the GL pipeline
    swap(vao, mVertexArray);
    mVertexCount = vertexCount;
}

void OpenGLStaticMesh::Draw(const std::shared_ptr<IShaderProgram>& program,
                            PrimitiveType primitiveType, std::size_t firstVertexIndex, std::size_t vertexCount)
{
    std::shared_ptr<OpenGLShaderProgram> programGL = std::static_pointer_cast<OpenGLShaderProgram>(program);
    mRenderer->SendDrawVertexArray(mVertexArray, programGL->GetFutureHandle(),
                                   ToGLPrimitiveType(primitiveType), firstVertexIndex, vertexCount);
}

std::size_t OpenGLStaticMesh::GetVertexCount() const
{
    return mVertexCount;
}

} // end namespace ng
