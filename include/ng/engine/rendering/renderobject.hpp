#ifndef NG_RENDEROBJECT_HPP
#define NG_RENDEROBJECT_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <memory>

namespace ng
{

class IMesh;

class RenderObject
{
public:
    std::shared_ptr<IMesh> Mesh;

    mat4 WorldTransform;
};

} // end namespace ng

#endif // NG_RENDEROBJECT_HPP
