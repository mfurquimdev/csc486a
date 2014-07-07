#include "ng/engine/opengl/openglcommands.hpp"

namespace ng
{

BeginFrameCommand::BeginFrameCommand(vec3 clearColor)
    : ClearColor(clearColor)
{ }

void BeginFrameCommand::Accept(IRendererCommandVisitor& visitor)
{
    visitor.Visit(*this);
}

void EndFrameCommand::Accept(IRendererCommandVisitor& visitor)
{
    visitor.Visit(*this);
}

RenderBatchCommand::RenderBatchCommand(RenderBatch batch)
    : Batch(std::move(batch))
{ }

void RenderBatchCommand::Accept(IRendererCommandVisitor& visitor)
{
    visitor.Visit(*this);
}

void QuitCommand::Accept(IRendererCommandVisitor& visitor)
{
    visitor.Visit(*this);
}

} // end namespace ng
