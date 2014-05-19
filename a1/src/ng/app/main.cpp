#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"
#include "ng/engine/windowmanager.hpp"
#include "ng/engine/windowevent.hpp"
#include "ng/engine/renderer.hpp"
#include "ng/engine/profiler.hpp"
#include "ng/engine/debug.hpp"
#include "ng/engine/staticmesh.hpp"
#include "ng/engine/vertexformat.hpp"

#include <chrono>
#include <sstream>

int main()
{
    auto windowManager = ng::CreateWindowManager();
    ng::VideoFlags videoFlags{};
    auto window = windowManager->CreateWindow("test", 640, 480, 0, 0, videoFlags);
    auto renderer = ng::CreateRenderer(windowManager, window);

    auto mesh = renderer->CreateStaticMesh();
    {
        ng::VertexFormat meshFormat;
        meshFormat.Position = ng::VertexAttribute(3, ng::ArithmeticType::Float,
                                                  false, 0, 0, true);
        static const float rawMeshData[] = {
            0.0f, 0.0f, 0.0f,
            1.0f, 1.0f,-1.0f,
            1.0f,-1.0f, 0.0f,
        };

        std::shared_ptr<const void> meshData(rawMeshData, [](const void*){});
        mesh->Init(meshFormat, std::move(meshData), sizeof(rawMeshData), nullptr, 0);
    }

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
        renderer->SwapBuffers();

        renderProfiler.Stop();
    }
}
