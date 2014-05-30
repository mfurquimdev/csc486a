#include "ng/engine/window/window.hpp"
#include "ng/engine/window/glcontext.hpp"
#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/windowevent.hpp"
#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/util/profiler.hpp"
#include "ng/engine/util/debug.hpp"
#include "ng/engine/rendering/staticmesh.hpp"
#include "ng/engine/rendering/vertexformat.hpp"
#include "ng/engine/rendering/shaderprogram.hpp"

#include "ng/framework/scenegraph/scenegraph.hpp"
#include "ng/framework/scenegraph/renderobjectnode.hpp"
#include "ng/framework/scenegraph/camera.hpp"
#include "ng/framework/scenegraph/gridmesh.hpp"
#include "ng/framework/scenegraph/uvsphere.hpp"
#include "ng/framework/scenegraph/linestrip.hpp"
#include "ng/framework/scenegraph/cubemesh.hpp"
#include "ng/framework/animation/catmullromspline.hpp"
#include "ng/engine/math/constants.hpp"

#include <chrono>
#include <vector>
#include <sstream>
#include <string>

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

void RebuildCatmullRomSpline(int numDivisions, const ng::CatmullRomSpline<float>& spline,
                             ng::LineStrip& lineStrip)
{
    lineStrip.Reset();

    if (spline.ControlPoints.size() >= 4)
    {
        std::size_t numSegments = spline.ControlPoints.size() - 4 + 1;

        for (std::size_t segment = 0; segment < numSegments; segment++)
        {
            for (int division = 0; division <= numDivisions; division++)
            {
                lineStrip.AddPoint(spline.CalculatePosition(segment, (float) division / numDivisions));
            }
        }
    }
}

int main() try
{
    // set up rendering
    std::shared_ptr<ng::IWindowManager> windowManager = ng::CreateWindowManager();
    std::shared_ptr<ng::IWindow> window = windowManager->CreateWindow("Splines", 640, 480, 0, 0, ng::VideoFlags());
    std::shared_ptr<ng::IRenderer> renderer = ng::CreateRenderer(windowManager, window);

    static const char* vsrc = "#version 130\n"
                              "uniform mat4 uModelView; uniform mat4 uProjection;"
                              "in vec4 iPosition;"
                              "out vec3 fViewPosition;"
                              "void main() {"
                              "    fViewPosition = vec3(uModelView * iPosition);"
                              "    gl_Position = uProjection * uModelView * iPosition;"
                              "}";

    static const char* fsrc = "#version 130\n"
                              "uniform vec4 uTint;"
                              "in vec3 fViewPosition;"
                              "out vec4 oColor;"
                              "void main() {"
                              "    oColor = uTint * pow(20.0f / length(fViewPosition),2);"
                              "}";

    std::shared_ptr<ng::IShaderProgram> program = renderer->CreateShaderProgram();

    program->Init(std::shared_ptr<const char>(vsrc, [](const char*){}),
                  std::shared_ptr<const char>(fsrc, [](const char*){}),
                  true);

    auto status = program->GetStatus();
    if (!status.first)
    {
        ng::DebugPrintf("Status error: %s\n", status.second.c_str());
        return -1;
    }

    ng::RenderState renderState;

    renderState.DepthTestEnabled = true;
    renderState.ActivatedParameters.set(ng::RenderState::Activate_DepthTestEnabled);

    renderState.PolygonMode = ng::PolygonMode::Line;
    renderState.ActivatedParameters.set(ng::RenderState::Activate_PolygonMode);

    // prepare the scene
    ng::SceneGraph roManager;

    std::shared_ptr<ng::Camera> camera = std::make_shared<ng::Camera>();
    std::shared_ptr<ng::CameraNode> cameraNode = std::make_shared<ng::CameraNode>(camera);
    cameraNode->SetPerspectiveProjection(70.0f, (float) window->GetWidth() / window->GetHeight(), 0.1f, 1000.0f);
    ng::vec3 eyePosition{ 10.0f, 10.0f, 10.0f };
    ng::vec3 eyeTarget{ 0.0f, 0.0f, 0.0f };
    ng::vec3 eyeUpVector{ 0.0f, 1.0f, 0.0f };
    cameraNode->SetLookAt(eyePosition, eyeTarget, eyeUpVector);
    cameraNode->SetViewport(0, 0, window->GetWidth(), window->GetHeight());
    roManager.SetCurrentCamera(cameraNode);

    std::shared_ptr<ng::GridMesh> gridMesh = std::make_shared<ng::GridMesh>(renderer);
    gridMesh->Init(20, 20, ng::vec2(1.0f));
    std::shared_ptr<ng::RenderObjectNode> gridNode = std::make_shared<ng::RenderObjectNode>(gridMesh);
    gridNode->SetLocalTransform(ng::Translate(-gridNode->GetLocalBoundingBox().GetCenter()));
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

    int numSplineDivisions = 10;

    bool isSelectedNodeBeingDragged = false;

    enum class ControlMode
    {
        Parametric,
        ConstantSpeed,
        EaseInEaseOut
    };

    ControlMode currentControlMode = ControlMode::Parametric;

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

                    cameraNode->SetLookAt(eyePosition, eyeTarget, eyeUpVector);
                }
                else if (e.Motion.ButtonStates[int(ng::MouseButton::Left)])
                {
                    if (isSelectedNodeBeingDragged)
                    {
                        ng::Ray<float> clickRay = RayFromClick(*cameraNode, *window, ng::ivec2(e.Motion.X, e.Motion.Y));

                        std::shared_ptr<ng::RenderObjectNode> selected = selectorCubeNode->GetParent().lock();

                        // move the selected node by picking a point on its xz plane.
                        ng::Plane<float> selectedNodePlane(gridNode->GetNormalMatrix() * gridMesh->GetNormal(),
                                                           selected->GetWorldBoundingBox().GetCenter());

                        // Check if ray and grid intersect
                        float t;
                        if (ng::RayPlaneIntersect(clickRay, selectedNodePlane, std::numeric_limits<float>::epsilon(), std::numeric_limits<float>::infinity(), t))
                        {
                            // collided with plane
                            ng::vec3 collisonPoint = clickRay.Origin + t * clickRay.Direction;

                            selected->SetLocalTransform(ng::Translate(collisonPoint));

                            // get the parent of the node to remove
                            std::shared_ptr<ng::RenderObjectNode> parentOfSelected = selected->GetParent().lock();
                            if (parentOfSelected == nullptr)
                            {
                                throw std::logic_error("Selected node should have a parent that contains it.");
                            }

                            // get the index of the selected node in its parent's children
                            std::vector<std::shared_ptr<ng::RenderObjectNode>> controlPointList = parentOfSelected->GetChildren();
                            std::size_t indexToMove = std::distance(controlPointList.begin(), std::find(controlPointList.begin(), controlPointList.end(), selected));

                            // update spline control point
                            catmullRomSpline.ControlPoints[indexToMove] = collisonPoint;

                            // remove that index from the line strip
                            lineStrip->SetPoint(lineStrip->GetPoints().begin() + indexToMove, collisonPoint);

                            // rebuild the catmull rom spline
                            RebuildCatmullRomSpline(numSplineDivisions, catmullRomSpline, *catmullRomStrip);
                        }
                    }
                }
            }
            else if (e.Type == ng::WindowEventType::KeyPress)
            {
                if (e.KeyPress.Scancode == ng::Scancode::Esc
                        && !isSelectedNodeBeingDragged)
                {
                    std::shared_ptr<ng::RenderObjectNode> selected = selectorCubeNode->GetParent().lock();
                    if (selected)
                    {
                        selected->AbandonChild(selectorCubeNode);
                        selectorCubeNode->Hide();
                    }
                }

                if (e.KeyPress.Scancode == ng::Scancode::Delete
                        && !isSelectedNodeBeingDragged)
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
                        RebuildCatmullRomSpline(numSplineDivisions, catmullRomSpline, *catmullRomStrip);

                        // remove the deleted node from its parent
                        parentOfSelected->AbandonChild(selected);

                        // reset the spline rider
                        // splineRiderT = 0.0f;

                        // if there are other points, try to reselect a useful one
                        if (catmullRomSpline.ControlPoints.size() > 0)
                        {
                            std::size_t indexToReselect = indexToDelete == 0 ? 0 : indexToDelete - 1;
                            controlPointGroupNode->GetChildren().at(indexToReselect)->AdoptChild(selectorCubeNode);
                            selectorCubeNode->Show();
                        }
                    }
                }
                else if (e.KeyPress.Scancode == ng::Scancode::Tab)
                {
                    currentControlMode = currentControlMode == ControlMode::Parametric ? ControlMode::ConstantSpeed
                                       : currentControlMode == ControlMode::ConstantSpeed ? ControlMode::EaseInEaseOut
                                       : currentControlMode == ControlMode::EaseInEaseOut ? ControlMode::Parametric
                                       : throw std::logic_error("Unhandled control mode");
                }
            }
            else if (e.Type == ng::WindowEventType::MouseButton)
            {
                if (e.Button.State == ng::ButtonState::Released && e.Button.Button == ng::MouseButton::Left)
                {
                    isSelectedNodeBeingDragged = false;
                }
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

                        isSelectedNodeBeingDragged = true;
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
                            RebuildCatmullRomSpline(numSplineDivisions, catmullRomSpline, *catmullRomStrip);
                        }
                    }
                }
            }
            else if (e.Type == ng::WindowEventType::MouseScroll)
            {
                if (isSelectedNodeBeingDragged)
                {
                    std::shared_ptr<ng::RenderObjectNode> selected = selectorCubeNode->GetParent().lock();

                    // calculate how much to go up or down
                    float delta = 0.2f * (e.Scroll.Direction > 0 ? 1.0f : -1.0f);

                    // move the node up or down
                    selected->SetLocalTransform(ng::Translate(0.0f, delta, 0.0f) * selected->GetLocalTransform());

                    // get the parent of the node to remove
                    std::shared_ptr<ng::RenderObjectNode> parentOfSelected = selected->GetParent().lock();
                    if (parentOfSelected == nullptr)
                    {
                        throw std::logic_error("Selected node should have a parent that contains it.");
                    }

                    // get the index of the selected node in its parent's children
                    std::vector<std::shared_ptr<ng::RenderObjectNode>> controlPointList = parentOfSelected->GetChildren();
                    std::size_t indexToMove = std::distance(controlPointList.begin(), std::find(controlPointList.begin(), controlPointList.end(), selected));

                    // update spline control point
                    catmullRomSpline.ControlPoints[indexToMove] = selected->GetWorldBoundingBox().GetCenter();

                    // remove that index from the line strip
                    lineStrip->SetPoint(lineStrip->GetPoints().begin() + indexToMove, selected->GetWorldBoundingBox().GetCenter());

                    // rebuild the catmull rom spline
                    RebuildCatmullRomSpline(numSplineDivisions, catmullRomSpline, *catmullRomStrip);
                }
                else
                {
                    float zoomDelta = 0.4f;

                    eyePosition = eyePosition + normalize(eyePosition - eyeTarget) * zoomDelta * (e.Scroll.Direction > 0 ? 1.0f : -1.0f);

                    cameraNode->SetLookAt(eyePosition, eyeTarget, eyeUpVector);
                }
            }
            else if (e.Type == ng::WindowEventType::WindowStructure)
            {
                cameraNode->SetViewport(0, 0, window->GetWidth(), window->GetHeight());
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

                splineRiderTSpeed = 3.0f;

                if (currentControlMode == ControlMode::ConstantSpeed)
                {
                    splineRiderTSpeed = 10.0f / ng::length(catmullRomSpline.CalculateVelocity(int(splineRiderT), splineRiderT - int(splineRiderT)));
                }

                splineRiderT = std::fmod(splineRiderT + splineRiderTSpeed * stepInSeconds, catmullRomSpline.ControlPoints.size() - 3);

                int segment = int(splineRiderT);
                float normalizedT = splineRiderT - segment;
                float splineParamValue = normalizedT;

                if (currentControlMode == ControlMode::EaseInEaseOut)
                {
                    splineParamValue = - 2 * normalizedT * normalizedT * normalizedT + 3 * normalizedT * normalizedT;
                }

                splineRiderNode->SetLocalTransform(ng::Translate(catmullRomSpline.CalculatePosition(segment, splineParamValue)));
            }
            else
            {
                splineRiderNode->Hide();
            }

            if (selectorCubeNode->GetParent().expired())
            {
                lineStripNode->Hide();
            }
            else
            {
                lineStripNode->Show();
            }

            lag -= fixedUpdateStep;
        }

        float arcLength = 0.0f;
        if (catmullRomSpline.ControlPoints.size() >= 4)
        {
            for (std::size_t segment = 0; segment < catmullRomSpline.ControlPoints.size() - 3; segment++)
            {
                arcLength += catmullRomSpline.GetArcLength(segment, 1.0f, numSplineDivisions);
            }
        }

        {
            std::stringstream ss;
            ss << "Arc length: " << arcLength;
            window->SetTitle(ss.str().c_str());
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
