#ifndef NG_RENDERBATCH_HPP
#define NG_RENDERBATCH_HPP

namespace ng
{

class IRenderBatch
{
public:
    virtual void Commit() = 0;

    virtual void SetShouldSwapBuffers(bool shouldSwapBuffers) = 0;
};

} // end namespace ng

#endif // NG_RENDERBATCH_HPP
