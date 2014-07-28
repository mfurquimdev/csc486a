#include "ng/engine/app.hpp"

#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/window.hpp"
#include "ng/engine/window/windowevent.hpp"

#include "ng/engine/filesystem/filesystem.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/scenegraph.hpp"
#include "ng/engine/rendering/material.hpp"

#include "ng/engine/math/constants.hpp"

#include "ng/engine/util/memory.hpp"
#include "ng/engine/util/scopeguard.hpp"
#include "ng/engine/util/debug.hpp"

#include "ng/framework/loaders/md5loader.hpp"

#include "ng/framework/meshes/skeletalmesh.hpp"
#include "ng/framework/meshes/md5mesh.hpp"
#include "ng/framework/meshes/basismesh.hpp"
#include "ng/framework/meshes/skeletonwireframemesh.hpp"

#include "ng/framework/models/skeletalmodel.hpp"
#include "ng/framework/models/md5model.hpp"

#include "ng/framework/textures/checkerboardtexture.hpp"

#include "ng/framework/util/fixedstepupdate.hpp"

#include <vector>
#include <chrono>

namespace a4
{

class A4 : public ng::IApp
{
    std::shared_ptr<ng::IWindowManager> mWindowManager;
    std::shared_ptr<ng::IWindow> mWindow;
    std::shared_ptr<ng::IRenderer> mRenderer;
    std::shared_ptr<ng::IFileSystem> mFileSystem;

    ng::SceneGraph mScene;
    std::shared_ptr<ng::SceneGraphCameraNode> mMainCamera;
    std::shared_ptr<ng::SceneGraphCameraNode> mOverlayCamera;

    std::shared_ptr<ng::SceneGraphNode> mAnimationNode;
    std::shared_ptr<ng::SceneGraphNode> mSkeletonNode;
    std::shared_ptr<ng::immutable<ng::Skeleton>> mAnimationSkeleton;
    std::shared_ptr<ng::IMesh> mAnimationBindPoseMesh;
    ng::MD5Anim mAnimationAnim;

    ng::FixedStepUpdate mFixedStepUpdate{std::chrono::milliseconds(1000/60)};

public:
    void Init() override
    {
        mWindowManager = ng::CreateWindowManager();

        mWindow = mWindowManager->CreateWindow(
            "a4", 640, 480, 0, 0, ng::VideoFlags());

        mRenderer = ng::CreateRenderer(mWindowManager, mWindow);

        mFileSystem = ng::CreateFileSystem();

        // setup materials
        ng::Material normalColoredMaterial(ng::MaterialType::NormalColored);
        ng::Material vertexColoredMaterial(ng::MaterialType::VertexColored);
        ng::Material wireframeMaterial(ng::MaterialType::Wireframe);

        ng::Material checkeredMaterial(ng::MaterialType::Textured);
        checkeredMaterial.Texture0 =
            std::make_shared<ng::CheckerboardTexture>(
                4, 4, 1, ng::vec4(1), ng::vec4(0));
        checkeredMaterial.Sampler0.MinFilter = ng::TextureFilter::Nearest;
        checkeredMaterial.Sampler0.MagFilter = ng::TextureFilter::Nearest;
        checkeredMaterial.Sampler0.WrapX = ng::TextureWrap::ClampToEdge;
        checkeredMaterial.Sampler0.WrapY = ng::TextureWrap::ClampToEdge;

        // setup scene
        std::shared_ptr<ng::SceneGraphNode> rootNode =
                std::make_shared<ng::SceneGraphNode>();
        mScene.Root = rootNode;

        mAnimationNode = std::make_shared<ng::SceneGraphNode>();

        {
            std::shared_ptr<ng::IReadFile> robotMD5MeshFile =
                    mFileSystem->GetReadFile("bob_lamp_update_export.md5mesh",
                                             ng::FileReadMode::Text);

            ng::MD5Model animationModel;
            ng::LoadMD5Mesh(animationModel, *robotMD5MeshFile);

            ng::Skeleton animationSkeleton(ng::Skeleton::FromMD5Model(animationModel));

            mAnimationSkeleton =
                    std::make_shared<ng::immutable<ng::Skeleton>>(
                        std::move(animationSkeleton));

            mAnimationBindPoseMesh =
                    std::make_shared<ng::MD5Mesh>(
                        std::move(animationModel));


            std::shared_ptr<ng::SceneGraphNode> bindPoseNode =
                std::make_shared<ng::SceneGraphNode>();
            bindPoseNode->Mesh = mAnimationBindPoseMesh;
            bindPoseNode->Material = normalColoredMaterial;
            mAnimationNode->Children.push_back(bindPoseNode);
        }

        {
            std::shared_ptr<ng::IReadFile> robotMD5AnimFile =
                    mFileSystem->GetReadFile("bob_lamp_update_export.md5anim",
                                             ng::FileReadMode::Text);

            ng::LoadMD5Anim(mAnimationAnim, *robotMD5AnimFile);
        }

        mAnimationNode->Material = wireframeMaterial;
        mAnimationNode->Transform = ng::mat4(
                                            1,0,0,0,
                                            0,0,-1,0,
                                            0,1,0,0,
                                            0,0,0,1);
        rootNode->Children.push_back(mAnimationNode);

        std::shared_ptr<ng::SceneGraphNode> basisNode1 =
                std::make_shared<ng::SceneGraphNode>();
        basisNode1->Mesh = std::make_shared<ng::BasisMesh>();
        basisNode1->Material = vertexColoredMaterial;
        rootNode->Children.push_back(basisNode1);
        std::shared_ptr<ng::SceneGraphNode> basisNode2 =
            std::make_shared<ng::SceneGraphNode>();
        basisNode2->Mesh = std::make_shared<ng::BasisMesh>();
        basisNode2->Material = vertexColoredMaterial;
        basisNode2->Transform = ng::translate4x4(0.0f,1.0f,0.0f);
        basisNode1->Children.push_back(basisNode2);
        std::shared_ptr<ng::SceneGraphNode> basisNode3 =
            std::make_shared<ng::SceneGraphNode>();
        basisNode3->Mesh = std::make_shared<ng::BasisMesh>();
        basisNode3->Material = vertexColoredMaterial;
        basisNode3->Transform = ng::translate4x4(0.0f,1.0f,0.0f);
        basisNode2->Children.push_back(basisNode3);
        std::shared_ptr<ng::SceneGraphNode> basisNode4 =
            std::make_shared<ng::SceneGraphNode>();
        basisNode4->Mesh = std::make_shared<ng::BasisMesh>();
        basisNode4->Material = vertexColoredMaterial;
        basisNode4->Transform = ng::translate4x4(0.0f,1.0f,0.0f);
        basisNode3->Children.push_back(basisNode4);

        mSkeletonNode = std::make_shared<ng::SceneGraphNode>();
        mSkeletonNode->Material = vertexColoredMaterial;
        mAnimationNode->Children.push_back(mSkeletonNode);

        std::shared_ptr<ng::SceneGraphNode> basisAtMinus4xThreez=
            std::make_shared<ng::SceneGraphNode>();
        basisAtMinus4xThreez->Mesh = std::make_shared<ng::BasisMesh>();
        basisAtMinus4xThreez->Material = vertexColoredMaterial;
        basisAtMinus4xThreez->Transform = ng::translate4x4(-4.0f,-0.5f,3.0f);
        mSkeletonNode->Children.push_back(basisAtMinus4xThreez);

        std::shared_ptr<ng::SceneGraphNode> basisAtMinus6xMinus9y=
            std::make_shared<ng::SceneGraphNode>();
        basisAtMinus6xMinus9y->Mesh = std::make_shared<ng::BasisMesh>();
        basisAtMinus6xMinus9y->Material = vertexColoredMaterial;
        basisAtMinus6xMinus9y->Transform = ng::translate4x4(-6.0f,-9.0f,5.0f);
        mSkeletonNode->Children.push_back(basisAtMinus6xMinus9y);

        mMainCamera = std::make_shared<ng::SceneGraphCameraNode>();
        rootNode->Children.push_back(mMainCamera);
        mScene.ActiveCameras.push_back(mMainCamera);

        // setup overlay
        std::shared_ptr<ng::SceneGraphNode> overlayRootNode =
                std::make_shared<ng::SceneGraphNode>();
        mScene.OverlayRoot = overlayRootNode;

        mOverlayCamera = std::make_shared<ng::SceneGraphCameraNode>();
        overlayRootNode->Children.push_back(mOverlayCamera);
        mScene.OverlayActiveCameras.push_back(mOverlayCamera);

        Update(std::chrono::milliseconds(0));
    }

    ng::AppStepAction Step() override
    {
        mFixedStepUpdate.QueuePendingSteps();

        while (mFixedStepUpdate.GetNumPendingSteps() > 0)
        {
            ng::WindowEvent we;
            while (mWindowManager->PollEvent(we))
            {
                if (we.Type == ng::WindowEventType::Quit)
                {
                    return ng::AppStepAction::Quit;
                }
                else
                {
                    HandleEvent(we);
                }
            }

            Update(mFixedStepUpdate.GetStepDuration());
            mFixedStepUpdate.Step();
        }

        {
            const ng::vec3 cornflowerBlue(
                        100.0f / 255.0f,
                        149.0f / 255.0f,
                        237.0f / 255.0f);

            mRenderer->BeginFrame(cornflowerBlue);
            auto endFrameScope = ng::make_scope_guard([&]{
                mRenderer->EndFrame();
            });

            mRenderer->Render(mScene);
        }

        return ng::AppStepAction::Continue;
    }

private:
    ng::vec3 mCameraPosition{14.0f};
    ng::vec3 mCameraTarget{0.0f};

    void HandleEvent(const ng::WindowEvent&)
    {

    }

    void UpdateCameraToWindow()
    {
        mMainCamera->Projection =
                ng::perspective(
                    ng::Radiansf(ng::Degreesf(70.0f)),
                    mWindow->GetAspect(),
                    0.1f, 1000.0f);

        mMainCamera->ViewportTopLeft = ng::ivec2(0,0);

        mMainCamera->ViewportSize = ng::ivec2(
                    mWindow->GetWidth(), mWindow->GetHeight());

        mOverlayCamera->Projection =
                ng::ortho2D(0.0f, (float) mWindow->GetWidth(),
                            0.0f, (float) mWindow->GetHeight());
        mOverlayCamera->ViewportTopLeft - ng::ivec2(0,0);
        mOverlayCamera->ViewportSize = ng::ivec2(
                    mWindow->GetWidth(), mWindow->GetHeight());
    }

    void UpdateCameraTransform(std::chrono::milliseconds dt)
    {
//        dt = std::chrono::milliseconds(0);
        dt /= 10;

        mCameraPosition = ng::vec3(ng::rotate4x4(ng::Radiansf(3.14f * dt.count() / 1000),
                                              0.0f, 1.0f, 0.0f)
                                 * ng::vec4(mCameraPosition,1.0f));

        mMainCamera->Transform = inverse(ng::lookAt(mCameraPosition,
                                                    mCameraTarget,
                                                    ng::vec3(0.0f,1.0f,0.0f)));
    }

    void Update(std::chrono::milliseconds dt)
    {
        UpdateCameraToWindow();
        UpdateCameraTransform(dt);

        static float frame = 0;

        frame = std::fmod(frame + 1, mAnimationAnim.Frames.size());

        ng::SkeletonLocalPose localAnimationPose(
                    ng::SkeletonLocalPose::FromMD5AnimFrame(
                        mAnimationSkeleton->get(), mAnimationAnim, frame));

        ng::SkeletonGlobalPose globalAnimationPose(
                    ng::SkeletonGlobalPose::FromLocalPose(
                        mAnimationSkeleton->get(), localAnimationPose));

        ng::SkinningMatrixPalette animationSkinningPalette =
                ng::SkinningMatrixPalette::FromGlobalPose(
                    mAnimationSkeleton->get(), globalAnimationPose);

        std::shared_ptr<ng::immutable<ng::SkinningMatrixPalette>>
                animationSkinningPalettePtr =
                    std::make_shared<ng::immutable<ng::SkinningMatrixPalette>>(
                        std::move(animationSkinningPalette));

        mAnimationNode->Mesh =
                std::make_shared<ng::SkeletalMesh>(
                    mAnimationBindPoseMesh,
                    animationSkinningPalettePtr);

        mSkeletonNode->Mesh =
                std::make_shared<ng::SkeletonWireframeMesh>(
                    mAnimationSkeleton,
                    animationSkinningPalettePtr);
    }
};

} // end namespace a4

namespace ng
{

std::shared_ptr<IApp> CreateApp()
{
    return std::shared_ptr<IApp>(new a4::A4());
}

} // end namespace ng
