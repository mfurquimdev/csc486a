#ifndef NG_RENDERSTATE_HPP
#define NG_RENDERSTATE_HPP

#include "ng/engine/math/linearalgebra.hpp"

#include <bitset>

namespace ng
{

enum class PolygonMode
{
    Point,
    Line,
    Fill
};

class RenderState
{
public:
    enum ActiveParamMask
    {
        Activate_DepthTestEnabled,
        Activate_PolygonMode,
        Activate_LineWidth,
        Activate_PointSize,
        Activate_Viewport,
        NumActivationParams
    };

    std::bitset<NumActivationParams> ActivatedParameters;

    bool DepthTestEnabled;

    ng::PolygonMode PolygonMode;

    float LineWidth;

    float PointSize;

    ivec4 Viewport;
};

} // end namespace ng

#endif // NG_RENDERSTATE_HPP
