#include "ng/framework/camera.hpp"

#include "ng/framework/renderobjectnode.hpp"

namespace ng
{

RenderObjectPass PerspectiveView::PreUpdate(std::chrono::milliseconds,
                                            RenderObjectNode& node)
{
    node.SetProjectionTransform(Perspective(mFieldOfViewY, mAspectRatio, mNearPlaneDistance, mFarPlaneDistance));

    return RenderObjectPass::Continue;
}

RenderObjectPass LookAtCamera::PreUpdate(std::chrono::milliseconds,
                                          RenderObjectNode& node)
{
    node.SetModelViewTransform(LookAt(mEyePosition, mCenterPosition, mUpVector));

    return RenderObjectPass::Continue;
}

} // end namespace ng
