#ifndef NG_RENDERSTATE_HPP
#define NG_RENDERSTATE_HPP

namespace ng
{

enum class PolygonMode
{
    Point,
    Line,
    Fill
};

struct RenderState
{
    bool DepthTestEnabled = false;
    ng::PolygonMode PolygonMode = ng::PolygonMode::Fill;
    float LineWidth = 1.0f;
    float PointSize = 1.0f;
};

} // end namespace ng

#endif // NG_RENDERSTATE_HPP
