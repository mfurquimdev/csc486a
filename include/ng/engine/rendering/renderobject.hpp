#ifndef NG_RENDEROBJECT_HPP
#define NG_RENDEROBJECT_HPP

#include <memory>

namespace ng
{

class IMesh;

class RenderObject
{
    std::shared_ptr<IMesh> mMesh;
public:

};

} // end namespace ng

#endif // NG_RENDEROBJECT_HPP
