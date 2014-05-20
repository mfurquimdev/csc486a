#ifndef NG_GLOBJECT_HPP
#define NG_GLOBJECT_HPP

#include "ng/engine/staticmesh.hpp"

#include "ng/engine/resource.hpp"
#include "ng/engine/vertexformat.hpp"

#include <GL/gl.h>

#include <memory>

namespace ng
{

class OpenGLRenderer;

class OpenGLBuffer
{
    std::shared_ptr<OpenGLRenderer> mRenderer;
    GLuint mHandle;

public:
    OpenGLBuffer();

    OpenGLBuffer(std::shared_ptr<OpenGLRenderer> renderer, GLuint handle);
    ~OpenGLBuffer();

    OpenGLBuffer(const OpenGLBuffer& other) = delete;
    OpenGLBuffer& operator=(const OpenGLBuffer& other) = delete;

    OpenGLBuffer(OpenGLBuffer&& other);
    OpenGLBuffer& operator=(OpenGLBuffer&& other);

    void swap(OpenGLBuffer& other);

    GLuint GetHandle() const
    {
        return mHandle;
    }
};

void swap(OpenGLBuffer& a, OpenGLBuffer& b);

class OpenGLStaticMesh : public IStaticMesh
{
public:
    std::shared_ptr<OpenGLRenderer> mRenderer;
    VertexFormat mVertexFormat;

    ResourceHandle mVertexBuffer;
    ResourceHandle mIndexBuffer;

    OpenGLStaticMesh(std::shared_ptr<OpenGLRenderer> renderer);

    void Init(const VertexFormat& format,
              std::shared_ptr<const void> vertexData,
              std::ptrdiff_t vertexDataSize,
              std::shared_ptr<const void> indexData,
              std::ptrdiff_t indexDataSize) override;
};

} // end namespace ng

#endif // NG_GLOBJECT_HPP
