#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"
#include "ng/engine/windowmanager.hpp"
#include "ng/engine/windowevent.hpp"
#include "ng/engine/renderer.hpp"
#include "ng/engine/profiler.hpp"
#include "ng/engine/debug.hpp"
#include "ng/engine/staticmesh.hpp"
#include "ng/engine/vertexformat.hpp"
#include "ng/engine/shaderprogram.hpp"

#include <chrono>

int main() try
{
    std::shared_ptr<ng::IWindowManager> windowManager = ng::CreateWindowManager();
    std::shared_ptr<ng::IWindow> window = windowManager->CreateWindow("test", 640, 480, 0, 0, ng::VideoFlags());
    std::shared_ptr<ng::IRenderer> renderer = ng::CreateRenderer(windowManager, window);

    auto mesh = renderer->CreateStaticMesh();
    {
        static const float rawMeshData[] = {
            0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f,
        };

        ng::VertexFormat meshFormat( {
                { ng::VertexAttributeName::Position, ng::VertexAttribute(3, ng::ArithmeticType::Float, false, 0, 0) }
            });

        std::shared_ptr<const void> meshData(rawMeshData, [](const float*){});
        mesh->Init(meshFormat, {
                       { ng::VertexAttributeName::Position, { std::move(meshData), sizeof(rawMeshData) } }
                   }, nullptr, 0, 3);
    }

    const char* vsrc = "#version 150\n"
                       "in vec4 iPosition; uniform mat4 uModelView; uniform mat4 uProjection;"
                       "void main() { gl_Position = uProjection * uModelView * iPosition; }";
    const char* fsrc = "#version 150\n"
                       "out vec4 oColor; uniform vec4 uTint;"
                       "void main() { oColor = uTint; }";
    std::shared_ptr<ng::IShaderProgram> program = renderer->CreateShaderProgram();
    program->Init(std::shared_ptr<const char>(vsrc, [](const char*){}),
                  std::shared_ptr<const char>(fsrc, [](const char*){}));

    ng::RenderState renderState;
    renderState.DepthTestEnabled = true;

    std::chrono::time_point<std::chrono::high_resolution_clock> now, then;
    then = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds lag(0);

    ng::Profiler renderProfiler;

    while (true)
    {
        now = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - then);
        then = now;
        lag += dt;

        ng::WindowEvent e;
        while (windowManager->PollEvent(e))
        {
            if (e.Type == ng::WindowEventType::Quit)
            {
                ng::DebugPrintf("Time spent rendering clientside: %lfms\n", renderProfiler.GetTotalTimeMS());
                ng::DebugPrintf("Average clientside rendering time per frame: %lfms\n", renderProfiler.GetAverageTimeMS());
                return 0;
            }
            else if (e.Type == ng::WindowEventType::MouseMotion)
            {
                ng::DebugPrintf("Motion: (%d, %d)\n", e.Motion.X, e.Motion.Y);
            }
            else if (e.Type == ng::WindowEventType::MouseButton)
            {
                ng::DebugPrintf("%s button %s at (%d, %d)\n",
                                ButtonStateToString(e.Button.State),
                                MouseButtonToString(e.Button.Button),
                                e.Button.X, e.Button.Y);
            }
            else if (e.Type == ng::WindowEventType::MouseScroll)
            {
                ng::DebugPrintf("Scrolled %s\n", e.Scroll.Direction > 0 ? "up" : "down");
            }
        }

        while (lag >= std::chrono::milliseconds(1000/60))
        {
            // TODO: update(1000/60 milliseconds)
            lag -= std::chrono::milliseconds(1000/60);
        }

        renderProfiler.Start();

        ng::mat4 modelview = ng::LookAt<float>({ 3.0f, 3.0f, 3.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        ng::mat4 projection = ng::Perspective(70.0f, (float) window->GetWidth() / window->GetHeight(), 0.1f, 1000.0f);

        renderer->Clear(true, true, false);
        mesh->Draw(
                    program,
                    {
                        { "uTint", ng::vec4(0,1,0,1) },
                        { "uModelView", modelview },
                        { "uProjection", projection }
                    },
                    renderState,
                    ng::PrimitiveType::Triangles, 0, mesh->GetVertexCount());
        renderer->SwapBuffers();

        renderProfiler.Stop();
    }
}
catch (const std::exception& e)
{
    ng::DebugPrintf("Caught fatal exception: %s\n", e.what());
}
catch (...)
{
    ng::DebugPrintf("Caught fatal unknown exception.");
}
