#ifndef NG_RENDEROBJECTNODE_HPP
#define NG_RENDEROBJECTNODE_HPP

#include "ng/engine/math/geometry.hpp"

#include "ng/framework/scenegraph/material.hpp"

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

    Material mMaterial;

    bool mIsHidden = false;

    mat4 mLocalTransform;

    mutable mat4 mWorldTransform;
    mutable bool mIsWorldTransformDirty = true;

    void SetDirtyRecursively() const;

protected:
    virtual bool IsCamera()
    {
        return false;
    }

    virtual bool IsLight()
    {
        return false;
    }

public:
    RenderObjectNode() = default;

    virtual ~RenderObjectNode() = default;

    RenderObjectNode(std::shared_ptr<IRenderObject> renderObject)
        : mRenderObject(std::move(renderObject))
    { }

    bool IsHidden() const
    {
        return mIsHidden;
    }

    void Hide()
    {
        mIsHidden = true;
    }

    void Show()
    {
        mIsHidden = false;
    }

    void SetMaterial(Material material)
    {
        mMaterial = std::move(material);
    }

    const Material& GetMaterial() const
    {
        return mMaterial;
    }

    mat4 GetLocalTransform() const
    {
        return mLocalTransform;
    }

    void SetLocalTransform(mat4 localTransform);

    mat4 GetWorldTransform() const;

    mat3 GetNormalMatrix() const;

    AxisAlignedBoundingBox<float> GetLocalBoundingBox() const;

    AxisAlignedBoundingBox<float> GetWorldBoundingBox() const;

    const std::shared_ptr<IRenderObject>& GetRenderObject() const
    {
        return mRenderObject;
    }

    virtual void SetRenderObject(std::shared_ptr<IRenderObject> renderObject)
    {
        mRenderObject = std::move(renderObject);
    }

    std::weak_ptr<RenderObjectNode> GetParent() const
    {
        return mParentNode;
    }

    std::vector<std::shared_ptr<RenderObjectNode>> GetChildren() const
    {
        return mChildNodes;
    }

    void AdoptChild(std::shared_ptr<RenderObjectNode> childNode);

    void AbandonChild(std::shared_ptr<RenderObjectNode> childNode);
};

} // end namespace ng

#endif // NG_RENDEROBJECTNODE_HPP
