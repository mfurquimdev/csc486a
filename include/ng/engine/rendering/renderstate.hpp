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
    enum ActiveParamMask
    {
        Activate_DepthTestEnabled,
        Activate_DepthTestFunc,

        Activate_BlendingEnabled,
        Activate_SourceBlendMode,
        Activate_DestinationBlendMode,
        Activate_BlendColor,

        Activate_PolygonMode,
        Activate_LineWidth,
        Activate_PointSize,
        Activate_Viewport,

        NumActivationParams
    };

    std::bitset<NumActivationParams> ActivatedParameters;

    bool DepthTestEnabled;

    // if this is not activated but DepthTestEnabled is,
    // then the DepthTestFunc will default to Less
    // if this is activated and DepthTestEnabled is not activated or false,
    // then this does nothing.
    ng::DepthTestFunc DepthTestFunc;

    bool BlendingEnabled;

    // if this is not activated but BlendingEnabled is,
    // then SourceBlendMode will default to One
    // if this is activated and BlendingEnabled is not activated or false,
    // then this does nothing.
    BlendMode SourceBlendMode;

    // Same as SourceBlendMode, but defaults to Zero.
    BlendMode DestinationBlendMode;

    // for use with blend modes: ConstantColor, OneMinusConstantColor, ConstantAlpha, OneMinusConstantAlpha
    // if not enabled, defaults to vec4(0,0,0,0)
    vec4 BlendColor;

    ng::PolygonMode PolygonMode;

    float LineWidth;

    float PointSize;

    ivec4 Viewport;
};

} // end namespace ng

#endif // NG_RENDERSTATE_HPP
