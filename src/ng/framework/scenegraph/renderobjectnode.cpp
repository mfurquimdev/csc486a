#include "ng/framework/scenegraph/renderobjectnode.hpp"

#include "ng/framework/scenegraph/renderobject.hpp"
#include "ng/engine/math/geometry.hpp"

namespace ng
{

void RenderObjectNode::SetDirtyRecursively() const
{
    if (!mIsWorldTransformDirty)
    {
        mIsWorldTransformDirty = true;
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
             !parent.expired();
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
    return mat3(inverse(GetWorldTransform()));
}

AxisAlignedBoundingBox<float> RenderObjectNode::GetLocalBoundingBox() const
{
    return GetRenderObject() ? GetRenderObject()->GetLocalBoundingBox() : AxisAlignedBoundingBox<float>();
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
    mChildNodes.push_back(childNode);
    childNode->SetDirtyRecursively();
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
    childNode->SetDirtyRecursively();
}

} // end namespace ng
