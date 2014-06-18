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

enum class DepthTestFunc
{
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

enum class BlendMode
{
    Zero,
    One,
    SourceColor,
    OneMinusSourceColor,
    DestinationColor,
    OneMinusDestinationColor,
    SourceAlpha,
    OneMinusSourceAlpha,
    DestinationAlpha,
    OneMinusDestinationAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,

    // only valid for SourceBlendMode
    SourceAlphaSaturate
};

class RenderState
{
public:
    bool DepthTestEnabled = false;

    ng::DepthTestFunc DepthTestFunc = ng::DepthTestFunc::Less;

    bool BlendingEnabled = false;

    BlendMode SourceBlendMode = BlendMode::One;

    BlendMode DestinationBlendMode = BlendMode::Zero;

    // for use with blend modes: ConstantColor, OneMinusConstantColor, ConstantAlpha, OneMinusConstantAlpha
    vec4 BlendColor = vec4(0,0,0,0);

    // note: not available on all platforms
    ng::PolygonMode PolygonMode = ng::PolygonMode::Fill;

    // note: not available on all platforms
    float LineWidth = 1;

    // note: not available on all platforms
    float PointSize = 1;

    // this value should likely be overwritten before drawing.
    ivec4 Viewport = ivec4(0,0,0,0);
};

} // end namespace ng

#endif // NG_RENDERSTATE_HPP
