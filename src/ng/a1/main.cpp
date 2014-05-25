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
#include "ng/framework/uvsphere.hpp"
#include "ng/framework/linestrip.hpp"
#include "ng/framework/cubemesh.hpp"

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
    renderState.ActivatedParameters.set(ng::RenderState::Activate_DepthTestEnabled);

    renderState.PolygonMode = ng::PolygonMode::Line;
    renderState.ActivatedParameters.set(ng::RenderState::Activate_PolygonMode);

    // prepare the scene
    ng::RenderObjectManager roManager;

    std::shared_ptr<ng::Camera> camera = std::make_shared<ng::Camera>();
    std::shared_ptr<ng::CameraNode> cameraNode = std::make_shared<ng::CameraNode>(camera);
    cameraNode->SetPerspectiveProjection(70.0f, (float) window->GetWidth() / window->GetHeight(), 0.1f, 1000.0f);
    cameraNode->SetLookAt({10.0f, 10.0f, 10.0f}, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
    cameraNode->SetViewport(0, 0, window->GetWidth(), window->GetHeight());
    roManager.SetCurrentCamera(cameraNode);

    std::shared_ptr<ng::GridMesh> gridMesh = std::make_shared<ng::GridMesh>(renderer);
    gridMesh->Init(10, 10, ng::vec2(1.0f,1.0f));
    std::shared_ptr<ng::RenderObjectNode> gridNode = std::make_shared<ng::RenderObjectNode>(gridMesh);
    gridNode->SetLocalTransform(ng::Translate(-5.0f, 0.0f, -5.0f));
    cameraNode->AdoptChild(gridNode);

    std::shared_ptr<ng::LineStrip> lineStrip = std::make_shared<ng::LineStrip>(renderer);
    std::shared_ptr<ng::RenderObjectNode> lineStripNode = std::make_shared<ng::RenderObjectNode>(lineStrip);
    cameraNode->AdoptChild(lineStripNode);

    std::shared_ptr<ng::CubeMesh> selectorCube = std::make_shared<ng::CubeMesh>(renderer);
    std::shared_ptr<ng::RenderObjectNode> selectorCubeNode = std::make_shared<ng::RenderObjectNode>(selectorCube);
    selectorCubeNode->Hide();

    // do an initial update to get things going
    roManager.Update(std::chrono::milliseconds(0));

    // initialize time stuff
    std::chrono::time_point<std::chrono::high_resolution_clock> now, then;
    then = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds lag(0);
    const std::chrono::milliseconds fixedUpdateStep(1000/60);

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

                if (e.Button.State == ng::ButtonState::Pressed && e.Button.Button == ng::MouseButton::Left)
                {
                    // create ray from camera and unprojected coordinate
                    ng::mat4 worldView = inverse(cameraNode->GetWorldTransform());

                    ng::vec2 mouseCoord(e.Button.X, window->GetHeight() - e.Button.Y);

                    ng::vec3 nearUnProject = ng::UnProject(ng::vec3(mouseCoord, 0.0f),
                                                           worldView, cameraNode->GetProjection(),
                                                           cameraNode->GetViewport());

                    ng::vec3 farUnProject = ng::UnProject(ng::vec3(mouseCoord, 1.0f),
                                                          worldView, cameraNode->GetProjection(),
                                                          cameraNode->GetViewport());

                    ng::mat4 viewWorld = cameraNode->GetLocalTransform();

                    ng::Ray<float> clickRay(ng::vec3(viewWorld * ng::vec4(0,0,0,1)),
                                            farUnProject - nearUnProject);

                    // convert the grid's AABB into world space
                    ng::AxisAlignedBoundingBox<float> gridAABB = gridNode->GetWorldBoundingBox();

                    // Use the center of the AABB as the point on the plane
                    ng::vec3 pointOnPlane = gridAABB.GetCenter();

                    // convert the grid's normal vector into world space
                    ng::vec3 gridNormal = gridNode->GetNormalMatrix() * gridMesh->GetNormal();

                    // Create a plane from the grid's point and normal
                    ng::Plane<float> gridPlane(gridNormal, pointOnPlane);

                    // Check if ray and grid intersect
                    float t;
                    if (ng::RayPlaneIntersect(clickRay, gridPlane, std::numeric_limits<float>::epsilon(), std::numeric_limits<float>::infinity(), t))
                    {
                        // collided with plane

                        ng::vec3 collisonPoint = clickRay.Origin + t * clickRay.Direction;
                        ng::DebugPrintf("TODO: Spawn a control point at {%f %f %f}\n",
                                        collisonPoint.x, collisonPoint.y, collisonPoint.z);

                        std::shared_ptr<ng::UVSphere> sphere = std::make_shared<ng::UVSphere>(renderer);
                        sphere->Init(10, 5, 0.3f);
                        std::shared_ptr<ng::RenderObjectNode> sphereNode = std::make_shared<ng::RenderObjectNode>(sphere);
                        sphereNode->SetLocalTransform(ng::Translate(collisonPoint));
                        cameraNode->AdoptChild(sphereNode);

                        lineStrip->AddPoint(collisonPoint);

                        if (!selectorCubeNode->GetParent().expired())
                        {
                            selectorCubeNode->GetParent().lock()->AbandonChild(selectorCubeNode);
                        }

                        selectorCube->Init(sphere->GetRadius() * 2);
                        selectorCubeNode->Show();
                        sphereNode->AdoptChild(selectorCubeNode);
                    }
                }
            }
            else if (e.Type == ng::WindowEventType::MouseScroll)
            {
                ng::DebugPrintf("Scrolled %s\n", e.Scroll.Direction > 0 ? "up" : "down");
            }
        }

        // update perspective to window
        cameraNode->SetPerspectiveProjection(70.0f, (float) window->GetWidth() / window->GetHeight(), 0.1f, 1000.0f);

        // do fixed step updates
        while (lag >= fixedUpdateStep)
        {
            roManager.Update(fixedUpdateStep);

            lag -= fixedUpdateStep;
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
