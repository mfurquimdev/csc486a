#ifndef NG_OPENGLCOMMANDS_HPP
#define NG_OPENGLCOMMANDS_HPP

#include "ng/engine/rendering/renderobject.hpp"

#include <memory>

namespace ng
{

class BeginFrameCommand;
class EndFrameCommand;
class RenderObjectsCommand;
class QuitCommand;

class IRendererCommandVisitor
{
public:
    virtual ~IRendererCommandVisitor() = default;

    virtual void Visit(BeginFrameCommand& cmd) = 0;
    virtual void Visit(EndFrameCommand& cmd) = 0;
    virtual void Visit(RenderObjectsCommand& cmd) = 0;
    virtual void Visit(QuitCommand& cmd) = 0;

    virtual bool ShouldQuit() = 0;
};

class IRendererCommand
{
public:
    virtual ~IRendererCommand() = default;

    virtual void Accept(IRendererCommandVisitor& visitor) = 0;
};

class BeginFrameCommand : public IRendererCommand
{
public:
    void Accept(IRendererCommandVisitor& visitor) override
    {
        visitor.Visit(*this);
    }
};

class EndFrameCommand : public IRendererCommand
{
public:
    void Accept(IRendererCommandVisitor& visitor) override
    {
        visitor.Visit(*this);
    }
};

class RenderObjectsCommand : public IRendererCommand
{
public:
    std::unique_ptr<const RenderObject[]> RenderObjects;
    std::size_t NumRenderObjects;

    RenderObjectsCommand() = default;

    RenderObjectsCommand(
            std::unique_ptr<const RenderObject[]> renderObjects,
            std::size_t numRenderObjects)
        : RenderObjects(std::move(renderObjects))
        , NumRenderObjects(numRenderObjects)
    { }

    void Accept(IRendererCommandVisitor& visitor) override
    {
        visitor.Visit(*this);
    }
};

class QuitCommand : public IRendererCommand
{
public:
    void Accept(IRendererCommandVisitor& visitor) override
    {
        visitor.Visit(*this);
    }
};

} // end namespace ng

#endif // NG_OPENGLCOMMANDS_HPP
