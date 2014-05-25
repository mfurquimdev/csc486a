#ifndef NG_RENDEROBJECTNODE_HPP
#define NG_RENDEROBJECTNODE_HPP

#include "ng/engine/linearalgebra.hpp"

#include <memory>
#include <vector>
#include <algorithm>

namespace ng
{

class IRenderObject;

class RenderObjectNode : public std::enable_shared_from_this<RenderObjectNode>
{
    std::weak_ptr<RenderObjectNode> mParentNode;
    std::shared_ptr<IRenderObject> mRenderObject;
    std::vector<std::shared_ptr<RenderObjectNode>> mChildNodes;

    mat4 mProjectionTransform;
    mat4 mModelViewTransform;

public:
    RenderObjectNode() = default;

    RenderObjectNode(std::shared_ptr<IRenderObject> renderObject)
        : mRenderObject(std::move(renderObject))
    { }

    mat4 GetProjectionTransform() const
    {
        return mProjectionTransform;
    }

    void SetProjectionTransform(mat4 projectionTransform)
    {
        mProjectionTransform = projectionTransform;
    }

    mat4 GetModelViewTransform() const
    {
        return mModelViewTransform;
    }

    void SetModelViewTransform(mat4 modelViewTransform)
    {
        mModelViewTransform = modelViewTransform;
    }

    const std::shared_ptr<IRenderObject>& GetRenderObject() const
    {
        return mRenderObject;
    }

    void SetRenderObject(std::shared_ptr<IRenderObject> renderObject)
    {
        mRenderObject = std::move(renderObject);
    }

    const std::weak_ptr<RenderObjectNode>& GetParent() const
    {
        return mParentNode;
    }

    const std::vector<std::shared_ptr<RenderObjectNode>>& GetChildren() const
    {
        return mChildNodes;
    }

    void AdoptChild(std::shared_ptr<RenderObjectNode> childNode)
    {
        auto it = std::find(mChildNodes.begin(), mChildNodes.end(), childNode);
        if (it != mChildNodes.end())
        {
            throw std::logic_error("Child already adopted");
        }

        childNode->mParentNode = shared_from_this();
        mChildNodes.push_back(std::move(childNode));
    }

    void AbandonChild(std::shared_ptr<RenderObjectNode> childNode)
    {
        auto it = std::find(mChildNodes.begin(), mChildNodes.end(), childNode);
        if (it == mChildNodes.end())
        {
            throw std::logic_error("Can't abandon a node that isn't a child");
        }

        mChildNodes.erase(it);
    }
};

} // end namespace ng

#endif // NG_RENDEROBJECTNODE_HPP
