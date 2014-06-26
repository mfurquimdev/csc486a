#ifndef NG_GLRENDERER_HPP
#define NG_GLRENDERER_HPP

#include "ng/engine/rendering/renderer.hpp"

#include <thread>
#include <mutex>

namespace ng
{

class IWindowManager;
class IWindow;
class OpenGLRenderBatch;

class RenderingThreadData
{
public:
    const std::shared_ptr<IWindowManager> WindowManager;
    const std::shared_ptr<IWindow> Window;

    std::mutex BatchConsumerLock;
    std::mutex BatchProducerLock;

    bool ShouldQuit;

    std::shared_ptr<const OpenGLRenderBatch> CurrentBatch;

    RenderingThreadData(std::shared_ptr<IWindowManager> windowManager,
                        std::shared_ptr<IWindow> window);
};

class OpenGLRenderer : public IRenderer, public std::enable_shared_from_this<OpenGLRenderer>
{
    RenderingThreadData mRenderingThreadData;
    std::thread mRenderingThread;

public:
    friend class OpenGLRenderBatch;

    OpenGLRenderer(std::shared_ptr<IWindowManager> windowManager,
                   std::shared_ptr<IWindow> window);

    ~OpenGLRenderer();

    std::shared_ptr<IRenderBatch> CreateRenderBatch() override;

    MeshID AddMesh(std::unique_ptr<IMesh> mesh) override;
    void RemoveMesh(MeshID meshID) override;
};

} // end namespace ng

#endif // NG_GLRENDERER_HPP
