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

#include "ng/framework/meshes/cubemesh.hpp"
#include "ng/framework/meshes/squaremesh.hpp"
#include "ng/framework/meshes/loopsubdivisionmesh.hpp"
#include "ng/framework/meshes/skeletalmesh.hpp"
#include "ng/framework/meshes/nearestjointskinnedmesh.hpp"
#include "ng/framework/models/skeletalmodel.hpp"
#include "ng/framework/meshes/objmesh.hpp"

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

    std::shared_ptr<ng::SceneGraphNode> mRobotArmNode;
    std::shared_ptr<ng::ImmutableSkeleton> mRobotSkeleton;
    std::shared_ptr<ng::IMesh> mRobotBindPoseMesh;
    std::vector<ng::SkeletonJointPose> mRobotPose;

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

        mRobotArmNode = std::make_shared<ng::SceneGraphNode>();

        {
            ng::Skeleton robotSkeleton;
            {
                robotSkeleton.Joints.emplace_back();
                robotSkeleton.Joints.back().Parent = ng::SkeletonJoint::RootJointIndex;

                robotSkeleton.Joints.emplace_back();
                robotSkeleton.Joints[robotSkeleton.Joints.size() - 1].Parent = 0;

                robotSkeleton.Joints.emplace_back();
                robotSkeleton.Joints[robotSkeleton.Joints.size() - 1].Parent = 1;
            }

            // prepare the bind pose
            mRobotPose.emplace_back();

            mRobotPose.emplace_back();
            mRobotPose.back().Translation = ng::vec3(0,3,0);

            mRobotPose.emplace_back();
            mRobotPose.back().Translation = ng::vec3(0,2,0);

            if (mRobotPose.size() != robotSkeleton.Joints.size())
            {
                throw std::logic_error("Not enough poses for the bind pose");
            }

            ng::CalculateInverseBindPose(mRobotPose.data(),
                                         robotSkeleton.Joints.data(),
                                         robotSkeleton.Joints.size());

            ng::DebugPrintf("Inverse bind pose matrices:\n");
            for (std::size_t i = 0; i < robotSkeleton.Joints.size(); i++)
            {
                ng::DebugPrintf("Inverse bind pose %d:\n", i);

                for (int c = 0; c < 4; c++)
                {
                    for (int r = 0; r < 4; r++)
                    {
                        ng::DebugPrintf("%f ", robotSkeleton.Joints[i].InverseBindPose[c][r]);
                    }
                    ng::DebugPrintf("\n");
                }
            }

            mRobotSkeleton =
                    std::make_shared<ng::ImmutableSkeleton>(
                        std::move(robotSkeleton));
        }

        {
            ng::ObjShape robotBindPoseShape;
            std::shared_ptr<ng::IReadFile> robotBindPoseFile =
                    mFileSystem->GetReadFile("robotarm.obj",
                                             ng::FileReadMode::Text);

            ng::LoadObj(robotBindPoseShape, *robotBindPoseFile);

            mRobotBindPoseMesh =
                    std::make_shared<ng::ObjMesh>(
                        std::move(robotBindPoseShape));

//            mRobotBindPoseMesh =
//                    std::make_shared<ng::NearestJointSkinnedMesh>(
//                        mRobotBindPoseMesh,
//                        mRobotSkeleton);
        }

        mRobotArmNode->Material = checkeredMaterial;
        rootNode->Children.push_back(mRobotArmNode);

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
    ng::vec3 mCameraPosition{10.0f,10.0f,10.0f};
    ng::vec3 mCameraTarget{0.0f,0.0f,0.0f};

    void HandleEvent(const ng::WindowEvent&)
    {

    }

    void UpdateCameraToWindow()
    {
        mMainCamera->Projection =
                ng::perspective(
                    70.0f,
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
        dt = std::chrono::milliseconds(0);

        mCameraPosition = ng::vec3(ng::rotate4x4(3.14f * dt.count() / 1000,
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

        static float r = 0.0f;

        r += dt.count() / 1000.0f * 1.5f;

        // mRobotPose[2].Scale = ng::vec3(1,std::sin(r) + 1, 1);
        mRobotPose[0].Rotation =
                ng::Quaternionf::FromAxisAndRotation(ng::vec3(0,0,1), r / 2);
        mRobotPose[1].Rotation =
                ng::Quaternionf::FromAxisAndRotation(ng::vec3(0,1,0), r * 2);
        mRobotPose[2].Rotation =
                ng::Quaternionf::FromAxisAndRotation(ng::vec3(0,0,1), r * 4);

//        ng::mat3 m(mRobotPose[2].Rotation);
//        ng::vec3 a = m * ng::vec3(1,0,0);
//        ng::DebugPrintf("M:\n");
//        for (int c = 0; c < 3; c++) {
//            for (int r = 0; r < 3; r++) {
//                ng::DebugPrintf("%f ", m[c][r]);
//            }
//            ng::DebugPrintf("\n");
//        }
//        ng::DebugPrintf("A: %f %f %f\n", a.x, a.y, a.z);

//        mRobotPose[0].Rotation =
//                ng::Quaternionf::FromAxisAndRotation(ng::vec3(0,0,1), 0.5f);

//        mRobotPose[1].Rotation =
//                ng::Quaternionf::FromAxisAndRotation(ng::vec3(0,0,1), -1.0f);

//        mRobotPose[2].Rotation =
//                ng::Quaternionf::FromAxisAndRotation(ng::vec3(0,0,1), r);

        if (mRobotSkeleton->GetSkeleton().Joints.size() !=
            mRobotPose.size())
        {
            throw std::logic_error(
                        "Mismatch of number of joints "
                        "and number of joint poses");
        }

        std::vector<ng::mat4> globalRobotPoses(mRobotPose.size());

        ng::LocalPosesToGlobalPoses(
                    mRobotSkeleton->GetSkeleton().Joints.data(),
                    mRobotPose.data(), mRobotPose.size(),
                    globalRobotPoses.data());

        ng::DebugPrintf("Global poses:\n");
        for (std::size_t i = 0; i < globalRobotPoses.size(); i++)
        {
            ng::DebugPrintf("global pose %d:\n", i);

            for (int c = 0; c < 4; c++)
            {
                for (int r = 0; r < 4; r++)
                {
                    ng::DebugPrintf("%f ", globalRobotPoses[i][c][r]);
                }
                ng::DebugPrintf("\n");
            }
        }

        ng::SkinningMatrixPalette robotSkinningPalette;
        robotSkinningPalette.SkinningMatrices.resize(mRobotPose.size());

        ng::GlobalPosesToSkinningMatrices(
                    mRobotSkeleton->GetSkeleton().Joints.data(),
                    globalRobotPoses.data(), globalRobotPoses.size(),
                    robotSkinningPalette.SkinningMatrices.data());

        ng::DebugPrintf("Skinning palette:\n");
        for (std::size_t i = 0; i < robotSkinningPalette.SkinningMatrices.size(); i++)
        {
            ng::DebugPrintf("Palette %d:\n", i);

            for (int c = 0; c < 4; c++)
            {
                for (int r = 0; r < 4; r++)
                {
                    ng::DebugPrintf("%f ", robotSkinningPalette.SkinningMatrices[i][c][r]);
                }
                ng::DebugPrintf("\n");
            }
        }

        std::shared_ptr<ng::ImmutableSkinningMatrixPalette>
                immutableRobotSkinningPalette =
                    std::make_shared<ng::ImmutableSkinningMatrixPalette>(
                        std::move(robotSkinningPalette));

        mRobotArmNode->Mesh =
                std::make_shared<ng::SkeletalMesh>(
                    mRobotBindPoseMesh,
                    immutableRobotSkinningPalette);
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
