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
#include <sstream>
#include <iostream>

int main() try
{
    auto windowManager = ng::CreateWindowManager();
    auto window = windowManager->CreateWindow("test", 640, 480, 0, 0, ng::VideoFlags());
    auto renderer = ng::CreateRenderer(windowManager, window);

    auto mesh = renderer->CreateStaticMesh();
    {
        static const float rawMeshData[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
        };

        ng::VertexFormat meshFormat( {
                { ng::VertexAttributeName::Position, ng::VertexAttribute(2, ng::ArithmeticType::Float, false, 0, 0) }
            });

        std::shared_ptr<const void> meshData(rawMeshData, [](const float*){});
        mesh->Init(meshFormat, {
                       { ng::VertexAttributeName::Position, { std::move(meshData), sizeof(rawMeshData) } }
                   }, nullptr, 0, 3);
    }

    const char* vsrc = "#version 150\n in vec4 iPosition; void main() { gl_Position = iPosition; }";
    const char* fsrc = "#version 150\n out vec4 oColor; uniform vec4 uTint; void main() { oColor = uTint; }";
    auto program = renderer->CreateShaderProgram();
    program->Init(std::shared_ptr<const char>(vsrc, [](const char*){}),
                  std::shared_ptr<const char>(fsrc, [](const char*){}));

    ng::RenderState renderState;

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
            if (e.type == ng::WindowEventType::Quit)
            {
                ng::DebugPrintf("Time spent rendering clientside: %lfms\n", renderProfiler.GetTotalTimeMS());
                ng::DebugPrintf("Average clientside rendering time per frame: %lfms\n", renderProfiler.GetAverageTimeMS());
                return 0;
            }
            else if (e.type == ng::WindowEventType::MouseMotion)
            {
                ng::DebugPrintf("Motion: (%d, %d)\n", e.motion.x, e.motion.y);
            }
            else if (e.type == ng::WindowEventType::MouseButton)
            {
                std::stringstream ss;
                ss << e.button.state << " button " << e.button.button
                   << " at (" << e.button.x << ", " << e.button.y << ")";
                ng::DebugPrintf("%s\n", ss.str().c_str());
            }
        }

        while (lag >= std::chrono::milliseconds(1000/60))
        {
            // TODO: update(1000/60 milliseconds)
            lag -= std::chrono::milliseconds(1000/60);
        }

        renderProfiler.Start();

        renderer->Clear(true, true, false);
        mesh->Draw(
                    program,
                    { { "uTint", ng::vec4(0,1,0,1) } },
                    renderState,
                    ng::PrimitiveType::Triangles, 0, mesh->GetVertexCount());
        renderer->SwapBuffers();

        renderProfiler.Stop();
    }
}
catch (const std::exception& e)
{
    std::cerr << "Caught fatal exception: " << e.what() << std::endl;
}
catch (...)
{
    std::cerr << "Caught fatal unknown exception." << std::endl;
}
