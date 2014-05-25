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

#include "ng/framework/renderobjectmanager.hpp"
#include "ng/framework/renderobjectnode.hpp"
#include "ng/framework/camera.hpp"
#include "ng/framework/gridmesh.hpp"

#include <chrono>
#include <vector>

int main() try
{
    // set up rendering
    std::shared_ptr<ng::IWindowManager> windowManager = ng::CreateWindowManager();
    std::shared_ptr<ng::IWindow> window = windowManager->CreateWindow("test", 640, 480, 0, 0, ng::VideoFlags());
    std::shared_ptr<ng::IRenderer> renderer = ng::CreateRenderer(windowManager, window);

    static const char* vsrc = "#version 150\n"
                             "in vec4 iPosition; uniform mat4 uModelView; uniform mat4 uProjection;"
                             "void main() { gl_Position = uProjection * uModelView * iPosition; }";

    static const char* fsrc = "#version 150\n"
                              "out vec4 oColor; uniform vec4 uTint;"
                              "void main() { oColor = uTint; }";

    std::shared_ptr<ng::IShaderProgram> program = renderer->CreateShaderProgram();

    program->Init(std::shared_ptr<const char>(vsrc, [](const char*){}),
                  std::shared_ptr<const char>(fsrc, [](const char*){}));

    ng::RenderState renderState;
    renderState.DepthTestEnabled = true;
    renderState.PolygonMode = ng::PolygonMode::Line;

    // prepare the scene
    ng::RenderObjectManager roManager;
    std::shared_ptr<ng::RenderObjectNode> rootNode = roManager.AddRoot();

    std::shared_ptr<ng::PerspectiveView> perspectiveView = std::make_shared<ng::PerspectiveView>();
    std::shared_ptr<ng::RenderObjectNode> perspectiveNode = std::make_shared<ng::RenderObjectNode>(perspectiveView);
    rootNode->AdoptChild(perspectiveNode);

    std::shared_ptr<ng::LookAtCamera> lookAtCamera = std::make_shared<ng::LookAtCamera>();
    std::shared_ptr<ng::RenderObjectNode> lookAtNode = std::make_shared<ng::RenderObjectNode>(lookAtCamera);
    perspectiveNode->AdoptChild(lookAtNode);

    std::shared_ptr<ng::GridMesh> gridMesh = std::make_shared<ng::GridMesh>(renderer);
    gridMesh->Init(10, 10, ng::vec2(1.0f,1.0f));
    std::shared_ptr<ng::RenderObjectNode> gridNode = std::make_shared<ng::RenderObjectNode>(gridMesh);
    gridNode->SetModelViewTransform(ng::Translate(-5.0f, 0.0f, -5.0f));
    lookAtNode->AdoptChild(gridNode);

    // initialize time stuff
    std::chrono::time_point<std::chrono::high_resolution_clock> now, then;
    then = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds lag(1000/60); // to make sure things are updated on the first tick

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

        // update perspective to window
        perspectiveView->SetAspectRatio((float) window->GetWidth() / window->GetHeight());

        // update camera to input
        lookAtCamera->SetEyePosition(ng::vec3(10.0f, 10.0f, 10.0f));

        // do fixed step updates
        while (lag >= std::chrono::milliseconds(1000/60))
        {
            roManager.Update(std::chrono::milliseconds(1000/60));

            lag -= std::chrono::milliseconds(1000/60);
        }

        renderProfiler.Start();

        renderer->Clear(true, true, false);

        roManager.Draw(program, renderState);

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
