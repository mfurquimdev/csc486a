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

class OpenGLBufferHandle
{
    std::shared_ptr<OpenGLRenderer> mRenderer;
    GLuint mHandle;

public:
    OpenGLBufferHandle();
    ~OpenGLBufferHandle();

    OpenGLBufferHandle(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle);

    OpenGLBufferHandle(const OpenGLBufferHandle& other) = delete;
    OpenGLBufferHandle& operator=(const OpenGLBufferHandle& other) = delete;

    OpenGLBufferHandle(OpenGLBufferHandle&& other);
    OpenGLBufferHandle& operator=(OpenGLBufferHandle&& other);

    void swap(OpenGLBufferHandle& other);

    GLuint GetHandle() const;
};

void swap(OpenGLBufferHandle& a, OpenGLBufferHandle& b);

class OpenGLShaderHandle
{
    std::shared_ptr<OpenGLRenderer> mRenderer;
    GLuint mHandle;

public:
    OpenGLShaderHandle();
    ~OpenGLShaderHandle();

    OpenGLShaderHandle(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle);

    OpenGLShaderHandle(const OpenGLShaderHandle& other) = delete;
    OpenGLShaderHandle& operator=(const OpenGLShaderHandle& other) = delete;

    OpenGLShaderHandle(OpenGLShaderHandle&& other);
    OpenGLShaderHandle& operator=(OpenGLShaderHandle&& other);

    void swap(OpenGLShaderHandle& other);

    GLuint GetHandle() const;
};

void swap(OpenGLShaderHandle& a, OpenGLShaderHandle& b);

class OpenGLShaderProgramHandle
{
    std::shared_ptr<OpenGLRenderer> mRenderer;
    GLuint mHandle;

public:
    OpenGLShaderProgramHandle();
    ~OpenGLShaderProgramHandle();

    OpenGLShaderProgramHandle(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle);

    OpenGLShaderProgramHandle(const OpenGLShaderProgramHandle& other) = delete;
    OpenGLShaderProgramHandle& operator=(const OpenGLShaderProgramHandle& other) = delete;

    OpenGLShaderProgramHandle(OpenGLShaderProgramHandle&& other);
    OpenGLShaderProgramHandle& operator=(OpenGLShaderProgramHandle&& other);

    void swap(OpenGLShaderProgramHandle& other);

    GLuint GetHandle() const;
};

void swap(OpenGLShaderProgramHandle& a, OpenGLShaderProgramHandle& b);

class VertexArray
{
public:
    VertexFormat Format;

    std::shared_future<OpenGLBufferHandle> Position;
    std::shared_future<OpenGLBufferHandle> Texcoord0;
    std::shared_future<OpenGLBufferHandle> Texcoord1;
    std::shared_future<OpenGLBufferHandle> Normal;

    std::shared_future<OpenGLBufferHandle> Indices;

    std::size_t VertexCount;

    VertexArray() = default;

    VertexArray(const VertexFormat& format,
                std::vector<std::shared_future<OpenGLBufferHandle>> vertexBufferHandles,
                std::shared_future<OpenGLBufferHandle> indexBufferHandle,
                std::size_t vertexCount);
};

class OpenGLShaderProgram : public IShaderProgram
{
    std::shared_ptr<OpenGLRenderer> mRenderer;

    std::shared_future<OpenGLShaderHandle> mVertexShader;
    std::shared_future<OpenGLShaderHandle> mFragmentShader;
    std::shared_future<OpenGLShaderProgramHandle> mProgram;

public:
    OpenGLShaderProgram(std::shared_ptr<OpenGLRenderer> renderer);

    void Init(std::shared_ptr<const char> vertexShaderSource,
              std::shared_ptr<const char> fragmentShaderSource) override;

    std::pair<bool,std::string> GetStatus() const override;

    std::shared_future<OpenGLShaderProgramHandle> GetFutureHandle() const;
};

class OpenGLStaticMesh : public IStaticMesh
{
    std::shared_ptr<OpenGLRenderer> mRenderer;

    VertexArray mVertexArray;

public:
    OpenGLStaticMesh(std::shared_ptr<OpenGLRenderer> renderer);

    void Init(const VertexFormat& format,
              const std::vector<std::pair<std::shared_ptr<const void>,std::ptrdiff_t>>& vertexDataAndSize,
              std::shared_ptr<const void> indexData,
              std::ptrdiff_t indexDataSize,
              std::size_t vertexCount) override;

    void Draw(const std::shared_ptr<IShaderProgram>& program,
              PrimitiveType primitiveType, std::size_t firstVertexIndex, std::size_t vertexCount) override;

    std::size_t GetVertexCount() const override;
};

} // end namespace ng

#endif // NG_GLOBJECT_HPP
