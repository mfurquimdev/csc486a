#ifndef NG_GLOBJECT_HPP
#define NG_GLOBJECT_HPP

#include "ng/engine/staticmesh.hpp"
#include "ng/engine/shaderprogram.hpp"

#include "ng/engine/vertexformat.hpp"

#include <GL/gl.h>

#include <memory>
#include <future>

namespace ng
{

class OpenGLRenderer;

template<class ReleasePolicy>
class OpenGLObject
{
    std::shared_ptr<OpenGLRenderer> mRenderer;
    GLuint mHandle = 0;

public:
    OpenGLObject(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle)
        : mRenderer(std::move(renderer))
        , mHandle(handle)
    { }

    ~OpenGLObject()
    {
        ReleasePolicy::Release(mRenderer, mHandle);
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

    const std::shared_ptr<OpenGLRenderer>& GetRenderer() const
    {
        return mRenderer;
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

namespace detail
{
    struct BufferReleasePolicy
    {
        static void Release(std::shared_ptr<OpenGLRenderer>& renderer, GLuint handle);
    };

    struct VertexArrayReleasePolicy
    {
        static void Release(std::shared_ptr<OpenGLRenderer>& renderer, GLuint handle);
    };

    struct ShaderReleasePolicy
    {
        static void Release(std::shared_ptr<OpenGLRenderer>& renderer, GLuint handle);
    };

    struct ShaderProgramReleasePolicy
    {
        static void Release(std::shared_ptr<OpenGLRenderer>& renderer, GLuint handle);
    };

} // end namespace detail

using OpenGLBufferHandle = OpenGLObject<detail::BufferReleasePolicy>;
using OpenGLVertexArrayHandle = OpenGLObject<detail::VertexArrayReleasePolicy>;
using OpenGLShaderHandle = OpenGLObject<detail::ShaderReleasePolicy>;
using OpenGLShaderProgramHandle = OpenGLObject<detail::ShaderProgramReleasePolicy>;

class OpenGLShaderProgram : public IShaderProgram
{
    std::shared_ptr<OpenGLRenderer> mRenderer;

    std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>> mProgram;
    std::shared_future<std::shared_ptr<OpenGLShaderHandle>> mVertexShader;
    std::shared_future<std::shared_ptr<OpenGLShaderHandle>> mFragmentShader;

public:
    OpenGLShaderProgram(std::shared_ptr<OpenGLRenderer> renderer);

    void Init(std::shared_ptr<const char> vertexShaderSource,
              std::shared_ptr<const char> fragmentShaderSource) override;

    std::pair<bool,std::string> GetStatus() const override;

    std::shared_future<std::shared_ptr<OpenGLShaderProgramHandle>> GetFutureHandle() const;
};

class OpenGLStaticMesh : public IStaticMesh
{
    std::shared_ptr<OpenGLRenderer> mRenderer;

    std::shared_future<std::shared_ptr<OpenGLVertexArrayHandle>> mVertexArray;

    size_t mVertexCount;
    bool mIsIndexed;
    ArithmeticType mIndexType;

public:
    OpenGLStaticMesh(std::shared_ptr<OpenGLRenderer> renderer);

    void Init(VertexFormat format,
              std::map<VertexAttributeName,std::pair<std::shared_ptr<const void>,std::ptrdiff_t>> attributeDataAndSize,
              std::shared_ptr<const void> indexData,
              std::ptrdiff_t indexDataSize,
              std::size_t vertexCount) override;

    void Draw(const std::shared_ptr<IShaderProgram>& program,
              PrimitiveType primitiveType, std::size_t firstVertexIndex, std::size_t vertexCount) override;

    std::size_t GetVertexCount() const override;
};

} // end namespace ng

#endif // NG_GLOBJECT_HPP
