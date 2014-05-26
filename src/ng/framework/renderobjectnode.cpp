#include "ng/framework/renderobjectnode.hpp"

namespace ng
{

void RenderObjectNode::SetDirtyRecursively() const
{
    if (!mIsWorldTransformDirty)
    {
        mIsWorldTransformDirty = true;
        mIsNormalMatrixDirty = true;
        for (const std::shared_ptr<RenderObjectNode>& child : GetChildren())
        {
            child->SetDirtyRecursively();
        }
    }
}

void RenderObjectNode::SetLocalTransform(mat4 localTransform)
{
    SetDirtyRecursively();
    mLocalTransform = localTransform;
}

mat4 RenderObjectNode::GetWorldTransform() const
{
    if (mIsWorldTransformDirty)
    {
        mWorldTransform = mLocalTransform;

        for (std::weak_ptr<RenderObjectNode> parent = GetParent();
             !parent.expired() && parent.lock().get() != this && !parent.lock()->IsCamera();
             parent = parent.lock()->GetParent())
        {
            mWorldTransform = parent.lock()->GetLocalTransform() * mWorldTransform;

            if (!parent.lock()->mIsWorldTransformDirty)
            {
                break;
            }
        }

        mIsWorldTransformDirty = false;
    }

    return mWorldTransform;
}

mat3 RenderObjectNode::GetNormalMatrix() const
{
    if (mIsNormalMatrixDirty)
    {
        mNormalMatrix = mat3(transpose(inverse(GetWorldTransform())));

        mIsNormalMatrixDirty = false;
    }
    return mNormalMatrix;
}

AxisAlignedBoundingBox<float> RenderObjectNode::GetWorldBoundingBox() const
{
    AxisAlignedBoundingBox<float> boundingBox = GetLocalBoundingBox();
    mat4 worldTransform = GetWorldTransform();

    vec4 newMin = worldTransform * vec4(boundingBox.Minimum, 1.0f);
    vec4 newMax = worldTransform * vec4(boundingBox.Maximum, 1.0f);

    using std::min;
    using std::max;

    boundingBox.Minimum = vec3(min(newMin.x, newMax.x), min(newMin.y, newMax.y), min(newMin.z, newMax.z));
    boundingBox.Maximum = vec3(max(newMin.x, newMax.x), max(newMin.y, newMax.y), max(newMin.z, newMax.z));

    return boundingBox;
}

void RenderObjectNode::AdoptChild(std::shared_ptr<RenderObjectNode> childNode)
{
    auto it = std::find(mChildNodes.begin(), mChildNodes.end(), childNode);
    if (it != mChildNodes.end())
    {
        throw std::logic_error("Child already adopted");
    }

    childNode->mParentNode = shared_from_this();
    mChildNodes.push_back(std::move(childNode));
}

void RenderObjectNode::AbandonChild(std::shared_ptr<RenderObjectNode> childNode)
{
    auto it = std::find(mChildNodes.begin(), mChildNodes.end(), childNode);
    if (it == mChildNodes.end())
    {
        throw std::logic_error("Can't abandon a node that isn't a child");
    }

    childNode->mParentNode.reset();
    mChildNodes.erase(it);
}

} // end namespace ng
