#ifndef NG_OPENGLCOMMANDS_HPP
#define NG_OPENGLCOMMANDS_HPP

#include "ng/engine/rendering/renderbatch.hpp"

#include <memory>

namespace ng
{

class IRendererCommandVisitor;

class IRendererCommand
{
public:
    virtual ~IRendererCommand() = default;

    virtual void Accept(IRendererCommandVisitor& visitor) = 0;
};

class BeginFrameCommand : public IRendererCommand
{
public:
    vec3 ClearColor;

    BeginFrameCommand(vec3 clearColor);

    void Accept(IRendererCommandVisitor& visitor) override;
};

class EndFrameCommand : public IRendererCommand
{
public:
    void Accept(IRendererCommandVisitor& visitor) override;
};

class RenderBatchCommand : public IRendererCommand
{
public:
    RenderBatch Batch;

    RenderBatchCommand() = default;

    RenderBatchCommand(RenderBatch batch);

    void Accept(IRendererCommandVisitor& visitor) override;
};

class QuitCommand : public IRendererCommand
{
public:
    void Accept(IRendererCommandVisitor& visitor) override;
};

class IRendererCommandVisitor
{
public:
    virtual ~IRendererCommandVisitor() = default;

    virtual void Visit(BeginFrameCommand& cmd) = 0;
    virtual void Visit(EndFrameCommand& cmd) = 0;
    virtual void Visit(RenderBatchCommand& cmd) = 0;
    virtual void Visit(QuitCommand& cmd) = 0;

    virtual bool ShouldQuit() = 0;
};

} // end namespace ng

#endif // NG_OPENGLCOMMANDS_HPP
