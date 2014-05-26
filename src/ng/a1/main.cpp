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
#include "ng/framework/catmullromspline.hpp"
#include "ng/engine/constants.hpp"

#include <chrono>
#include <vector>

ng::Ray<float> RayFromClick(const ng::CameraNode& cameraNode,
                            const ng::IWindow& window,
                            ng::ivec2 click)
{
    // create ray from camera and unprojected coordinate
    ng::mat4 worldView = inverse(cameraNode.GetWorldTransform());

    ng::vec2 mouseCoord(click.x, window.GetHeight() - click.y);

    ng::vec3 nearUnProject = ng::UnProject(ng::vec3(mouseCoord, 0.0f),
                                           worldView, cameraNode.GetProjection(),
                                           cameraNode.GetViewport());

    ng::vec3 farUnProject = ng::UnProject(ng::vec3(mouseCoord, 1.0f),
                                          worldView, cameraNode.GetProjection(),
                                          cameraNode.GetViewport());

    ng::Ray<float> clickRay(ng::vec3(cameraNode.GetLocalTransform() * ng::vec4(0,0,0,1)),
                            farUnProject - nearUnProject);

    return clickRay;
}

void RebuildCatmullRomSpline(const ng::CatmullRomSpline<float>& spline,
                             ng::LineStrip& lineStrip)
{
    lineStrip.Reset();

    if (spline.ControlPoints.size() >= 4)
    {
        int numDivisions = 10;

        std::size_t numSegments = spline.ControlPoints.size() - 4 + 1;

        for (std::size_t segment = 0; segment < numSegments; segment++)
        {
            for (int division = 0; division <= numDivisions; division++)
            {
                lineStrip.AddPoint(spline.CalculatePoint(segment, (float) division / numDivisions));
            }
        }
    }
}

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
    ng::vec3 eyePosition{ 10.0f, 10.0f, 10.0f };
    cameraNode->SetLookAt(eyePosition, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
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

    ng::CatmullRomSpline<float> catmullRomSpline;
    std::shared_ptr<ng::LineStrip> catmullRomStrip = std::make_shared<ng::LineStrip>(renderer);
    std::shared_ptr<ng::RenderObjectNode> catmullStripNode = std::make_shared<ng::RenderObjectNode>(catmullRomStrip);
    cameraNode->AdoptChild(catmullStripNode);

    std::shared_ptr<ng::RenderObjectNode> controlPointGroupNode = std::make_shared<ng::RenderObjectNode>();
    cameraNode->AdoptChild(controlPointGroupNode);

    std::shared_ptr<ng::CubeMesh> selectorCube = std::make_shared<ng::CubeMesh>(renderer);
    std::shared_ptr<ng::RenderObjectNode> selectorCubeNode = std::make_shared<ng::RenderObjectNode>(selectorCube);
    selectorCubeNode->Hide();

    std::shared_ptr<ng::UVSphere> splineRider = std::make_shared<ng::UVSphere>(renderer);
    splineRider->Init(5, 3, 0.6f);
    std::shared_ptr<ng::RenderObjectNode> splineRiderNode = std::make_shared<ng::RenderObjectNode>(splineRider);
    cameraNode->AdoptChild(splineRiderNode);
    splineRiderNode->Hide();
    float splineRiderT = 0.0f;
    float splineRiderTSpeed = 3.0f; // units per second

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
                if (e.Motion.ButtonStates[int(ng::MouseButton::Right)])
                {
                    int deltaX = e.Motion.X - e.Motion.OldX;
                    float rotation = (float) - deltaX / window->GetWidth() * ng::pi<float>::value * 2;

                    eyePosition = ng::vec3(ng::Rotate(rotation, 0.0f, 1.0f, 0.0f) * ng::vec4(eyePosition, 0.0f));

                    cameraNode->SetLookAt(eyePosition, { 0, 0, 0 }, { 0, 1, 0 });
                }
            }
            else if (e.Type == ng::WindowEventType::KeyPress)
            {
                if (e.KeyPress.Scancode == ng::Scancode::Delete)
                {
                    // get the control point the selector is currently bound to
                    std::shared_ptr<ng::RenderObjectNode> selected = selectorCubeNode->GetParent().lock();
                    if (selected)
                    {
                        // unhook the selector from who it was previously hooked to
                        selected->AbandonChild(selectorCubeNode);
                        selectorCubeNode->Hide();

                        // get the parent of the node to remove
                        std::shared_ptr<ng::RenderObjectNode> parentOfSelected = selected->GetParent().lock();
                        if (parentOfSelected == nullptr)
                        {
                            throw std::logic_error("Selected node should have a parent that contains it.");
                        }

                        // get the index of the selected node in its parent's children
                        std::vector<std::shared_ptr<ng::RenderObjectNode>> controlPointList = parentOfSelected->GetChildren();
                        std::size_t indexToDelete = std::distance(controlPointList.begin(), std::find(controlPointList.begin(), controlPointList.end(), selected));

                        // remove that index from the catmull rom spline
                        catmullRomSpline.ControlPoints.erase(catmullRomSpline.ControlPoints.begin() + indexToDelete);

                        // remove that index from the line strip
                        lineStrip->RemovePoint(lineStrip->GetPoints().begin() + indexToDelete);

                        // rebuild the catmull rom spline
                        RebuildCatmullRomSpline(catmullRomSpline, *catmullRomStrip);

                        // remove the deleted node from its parent
                        parentOfSelected->AbandonChild(selected);

                        // reset the spline rider
                        splineRiderT = 0.0f;
                    }
                }
            }
            else if (e.Type == ng::WindowEventType::MouseButton)
            {
                if (e.Button.State == ng::ButtonState::Pressed && e.Button.Button == ng::MouseButton::Left)
                {

                    ng::Ray<float> clickRay = RayFromClick(*cameraNode, *window, ng::ivec2(e.Button.X, e.Button.Y));

                    float closestControlPointT = std::numeric_limits<float>::infinity();
                    std::shared_ptr<ng::RenderObjectNode> closestControlPoint;

                    // see if a click was made on any of the control points
                    for (std::shared_ptr<ng::RenderObjectNode> controlPoint : controlPointGroupNode->GetChildren())
                    {
                        ng::AxisAlignedBoundingBox<float> bbox = controlPoint->GetWorldBoundingBox();
                        ng::Sphere<float> sph(bbox.GetCenter(), std::max({
                                                                             (bbox.Maximum.x - bbox.Minimum.x) / 2,
                                                                             (bbox.Maximum.y - bbox.Minimum.y) / 2,
                                                                             (bbox.Maximum.z - bbox.Minimum.z) / 2
                                                                         }));

                        float t;
                        if (ng::RaySphereIntersect(clickRay, sph, std::numeric_limits<float>::epsilon(), std::numeric_limits<float>::infinity(), t))
                        {
                            if (t < closestControlPointT)
                            {
                                closestControlPoint = controlPoint;
                                closestControlPointT = t;
                            }
                        }
                    }

                    // handle collision with control point
                    if (closestControlPoint != nullptr)
                    {
                        ng::AxisAlignedBoundingBox<float> bbox = closestControlPoint->GetWorldBoundingBox();
                        ng::Sphere<float> sph(bbox.GetCenter(), std::max({
                                                                             (bbox.Maximum.x - bbox.Minimum.x) / 2,
                                                                             (bbox.Maximum.y - bbox.Minimum.y) / 2,
                                                                             (bbox.Maximum.z - bbox.Minimum.z) / 2
                                                                         }));

                        if (!selectorCubeNode->GetParent().expired())
                        {
                            selectorCubeNode->GetParent().lock()->AbandonChild(selectorCubeNode);
                        }

                        selectorCube->Init(sph.Radius * 2);
                        selectorCubeNode->Show();
                        closestControlPoint->AdoptChild(selectorCubeNode);
                    }
                    else
                    {
                        // otherwise, collide with the plane.

                        // Create a plane from the grid's normal and its center
                        ng::Plane<float> gridPlane(gridNode->GetNormalMatrix() * gridMesh->GetNormal(),
                                                   gridNode->GetWorldBoundingBox().GetCenter());

                        // Check if ray and grid intersect
                        float t;
                        if (ng::RayPlaneIntersect(clickRay, gridPlane, std::numeric_limits<float>::epsilon(), std::numeric_limits<float>::infinity(), t))
                        {
                            // collided with plane

                            ng::vec3 collisonPoint = clickRay.Origin + t * clickRay.Direction;

                            // create a sphere and sphere node to represent the newly added control point
                            std::shared_ptr<ng::UVSphere> sphere = std::make_shared<ng::UVSphere>(renderer);
                            sphere->Init(10, 5, 0.3f);
                            std::shared_ptr<ng::RenderObjectNode> sphereNode = std::make_shared<ng::RenderObjectNode>(sphere);
                            sphereNode->SetLocalTransform(ng::Translate(collisonPoint));

                            // add it to the node that hosts all control points
                            controlPointGroupNode->AdoptChild(sphereNode);

                            // add the new point to the line strip
                            lineStrip->AddPoint(collisonPoint);

                            // add the new point to the catmull rom spline
                            catmullRomSpline.ControlPoints.push_back(collisonPoint);

                            // unhook the selector from its previous parent
                            if (!selectorCubeNode->GetParent().expired())
                            {
                                selectorCubeNode->GetParent().lock()->AbandonChild(selectorCubeNode);
                            }

                            // update the selector to point to this new node
                            selectorCube->Init(sphere->GetRadius() * 2);
                            selectorCubeNode->Show();
                            sphereNode->AdoptChild(selectorCubeNode);

                            // rebuild the catmull rom mesh
                            RebuildCatmullRomSpline(catmullRomSpline, *catmullRomStrip);
                        }
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
            float stepInSeconds = fixedUpdateStep.count() / 1000.0f;

            roManager.Update(fixedUpdateStep);

            if (catmullRomSpline.ControlPoints.size() >= 4)
            {
                splineRiderNode->Show();
                splineRiderT = std::fmod(splineRiderT + splineRiderTSpeed * stepInSeconds, catmullRomSpline.ControlPoints.size() - 3);
                int segment = int(splineRiderT);
                splineRiderNode->SetLocalTransform(ng::Translate(catmullRomSpline.CalculatePoint(segment, splineRiderT - segment)));
            }
            else
            {
                splineRiderNode->Hide();
            }

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
