#ifndef NG_GLOBJECT_HPP
#define NG_GLOBJECT_HPP

#include "ng/engine/rendering/staticmesh.hpp"
#include "ng/engine/rendering/shaderprogram.hpp"

#include "ng/engine/rendering/vertexformat.hpp"

#include "ng/engine/util/synchronousfuture.hpp"

#include <GL/gl.h>

#include <memory>
#include <future>
#include <vector>

namespace ng
{

// hack job to make emscripten work, since it doesn't like std promise/future
#ifdef NG_USE_EMSCRIPTEN
template<class T>
using OpenGLFuture = synchronous_future<T>;
template<class T>
using OpenGLSharedFuture = synchronous_shared_future<T>;
template<class T>
using OpenGLPromise = synchronous_promise<T>;
#else
template<class T>
using OpenGLFuture = std::future<T>;
template<class T>
using OpenGLSharedFuture = std::shared_future<T>;
template<class T>
using OpenGLPromise = std::promise<T>;
#endif

class OpenGLRenderer;

template<class ObjectPolicy>
class OpenGLObject : public ObjectPolicy
{
    OpenGLRenderer* mRenderer;
    GLuint mHandle = 0;

public:
    using ObjectPolicy::AddDependents;

    OpenGLObject(OpenGLRenderer& renderer, GLuint handle)
        : mRenderer(&renderer)
        , mHandle(handle)
    { }

    ~OpenGLObject()
    {
        ObjectPolicy::Release(*mRenderer, mHandle);
    }

    OpenGLObject(const OpenGLObject& other) = delete;
    OpenGLObject& operator=(const OpenGLObject& other) = delete;

    OpenGLObject(OpenGLObject&& other)
    {
        swap(other);
    }

    OpenGLObject& operator=(OpenGLObject&& other)
    {
        if (this != &other)
        {
            OpenGLObject tmp(std::move(other));
            swap(tmp);
        }
        return *this;
    }

    void swap(OpenGLObject& other)
    {
        using std::swap;
        swap(mRenderer, other.mRenderer);
        swap(mHandle, other.mHandle);
    }

    const OpenGLRenderer& GetRenderer() const
    {
        return *mRenderer;
    }

    GLuint GetHandle() const
    {
        return mHandle;
    }
};

template<class ResourceTraits>
void swap(OpenGLObject<ResourceTraits>& a, OpenGLObject<ResourceTraits>& b)
{
    a.swap(b);
}

namespace openglobjectpolicies
{
    class BufferPolicy
    {
    public:
        void Release(OpenGLRenderer& renderer, GLuint handle);
        void AddDependents(); // no definition intentionally
    };

    class ShaderPolicy
    {
    public:
        void Release(OpenGLRenderer& renderer, GLuint handle);
        void AddDependents(); // no definition intentionally
    };

} // end namespace openglobjectpolicies

using OpenGLBufferHandle = OpenGLObject<openglobjectpolicies::BufferPolicy>;
using OpenGLShaderHandle = OpenGLObject<openglobjectpolicies::ShaderPolicy>;

namespace openglobjectpolicies
{
    class VertexArrayPolicy
    {
        std::vector<std::shared_ptr<OpenGLBufferHandle>> mArrayBuffers;
        std::shared_ptr<OpenGLBufferHandle> mElementArrayBuffer;

    public:
        void Release(OpenGLRenderer& renderer, GLuint handle);
        void AddDependents(std::vector<std::shared_ptr<OpenGLBufferHandle>> arrayBuffers,
                           std::shared_ptr<OpenGLBufferHandle> elementArrayBuffer);

        const std::shared_ptr<OpenGLBufferHandle>& GetElementArrayBuffer() const
        {
            return mElementArrayBuffer;
        }
    };

    class ShaderProgramPolicy
    {
        std::shared_ptr<OpenGLShaderHandle> mVertexShader;
        std::shared_ptr<OpenGLShaderHandle> mFragmentShader;

    public:
        void Release(OpenGLRenderer& renderer, GLuint handle);
        void AddDependents(std::shared_ptr<OpenGLShaderHandle> vertexShader,
                           std::shared_ptr<OpenGLShaderHandle> fragmentShader);
    };

} // end namespace openglobjectpolicies

using OpenGLVertexArrayHandle = OpenGLObject<openglobjectpolicies::VertexArrayPolicy>;
using OpenGLShaderProgramHandle = OpenGLObject<openglobjectpolicies::ShaderProgramPolicy>;

class OpenGLShaderProgram : public IShaderProgram
{
    std::shared_ptr<OpenGLRenderer> mRenderer;

    OpenGLSharedFuture<std::shared_ptr<OpenGLShaderProgramHandle>> mProgram;
    OpenGLSharedFuture<std::shared_ptr<OpenGLShaderHandle>> mVertexShader;
    OpenGLSharedFuture<std::shared_ptr<OpenGLShaderHandle>> mFragmentShader;

public:
    OpenGLShaderProgram(std::shared_ptr<OpenGLRenderer> renderer);

    void Init(std::shared_ptr<const char> vertexShaderSource,
              std::shared_ptr<const char> fragmentShaderSource,
              bool validate) override;

    std::pair<bool,std::string> GetStatus() const override;

    OpenGLSharedFuture<std::shared_ptr<OpenGLShaderProgramHandle>> GetFutureHandle() const;
};

class VertexArray
{
public:
    VertexFormat Format;

    OpenGLSharedFuture<std::shared_ptr<OpenGLVertexArrayHandle>> VertexArrayHandle;

    std::map<VertexAttributeName,OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>>> AttributeBuffers;
    OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>> IndexBuffer;

    std::size_t VertexCount = 0;

    VertexArray() = default;

    VertexArray(
        VertexFormat format,
        OpenGLSharedFuture<std::shared_ptr<OpenGLVertexArrayHandle>> vertexArrayHandle,
        std::map<VertexAttributeName,OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>>> attributeBuffers,
        OpenGLSharedFuture<std::shared_ptr<OpenGLBufferHandle>> indexBuffer,
        std::size_t vertexCount);
};


class OpenGLStaticMesh : public IStaticMesh
{
    std::shared_ptr<OpenGLRenderer> mRenderer;

    VertexArray mVertexArray;

public:
    OpenGLStaticMesh(std::shared_ptr<OpenGLRenderer> renderer);

    void Init(VertexFormat format,
              std::map<VertexAttributeName,std::pair<std::shared_ptr<const void>,std::ptrdiff_t>> attributeDataAndSize,
              std::shared_ptr<const void> indexData,
              std::ptrdiff_t indexDataSize,
              std::size_t vertexCount) override;

    void Draw(std::shared_ptr<IShaderProgram> program,
              std::map<std::string, UniformValue> uniforms,
              RenderState renderState,
              PrimitiveType primitiveType, std::size_t firstVertexIndex, std::size_t vertexCount) override;

    std::size_t GetVertexCount() const override;
};

} // end namespace ng

#endif // NG_GLOBJECT_HPP
