#include "ng/framework/scenegraph/shaderprofile.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/shaderprogram.hpp"

namespace ng
{

void ShaderProfile::BuildShaders(const std::shared_ptr<IRenderer>& renderer)
{
    static const char* phong_vsrc =
            "#version 100\n"

            "struct Light {\n"
            "    highp vec3 Position;\n"
            "    highp float Radius;\n"
            "    mediump vec3 Color;\n"
            "};\n"

            "uniform Light uLight;\n"
            "uniform highp mat4 uProjection;\n"
            "uniform highp mat4 uModelView;\n"

            "attribute highp vec4 iPosition;\n"
            "attribute mediump vec3 iNormal;\n"

            "varying mediump vec3 fLightTint;\n"

            "void main() {\n"
            "    vec3 P = iPosition.xyz;\n"
            "    vec3 L = normalize(uLight.Position - P);\n"
            "    fLightTint = uLight.Color * max(dot(iNormal, L),0.0);\n"
            "    gl_Position = uProjection * uModelView * iPosition;\n"
            "}\n";

    static const char* phong_fsrc =
            "#version 100\n"

            "uniform lowp vec3 uTint;\n"

            "varying lowp vec3 fLightTint;\n"

            "void main() {\n"
            "    gl_FragColor = vec4(fLightTint * uTint, 1.0);\n"
            "}\n";

    mPhongProgram = renderer->CreateShaderProgram();
    mPhongProgram->Init(std::shared_ptr<const char>(phong_vsrc, [](const char*){}),
                        std::shared_ptr<const char>(phong_fsrc, [](const char*){}),
                        true);

    static const char* debug_vsrc =
            "#version 100\n"

            "uniform highp mat4 uProjection;\n"
            "uniform highp mat4 uModelView;\n"

            "attribute highp vec4 iPosition;\n"

            "void main() {\n"
            "    gl_Position = uProjection * uModelView * iPosition;\n"
            "}\n";

    static const char* debug_fsrc =
            "#version 100\n"

            "uniform lowp vec3 uTint;\n"

            "void main() {\n"
            "    gl_FragColor = vec4(uTint,1.0);\n"
            "}\n";

    mDebugProgram = renderer->CreateShaderProgram();
    mDebugProgram->Init(std::shared_ptr<const char>(debug_vsrc, [](const char*){}),
                        std::shared_ptr<const char>(debug_fsrc, [](const char*){}),
                        true);
}

std::shared_ptr<IShaderProgram> ShaderProfile::GetProgramForMaterial(const Material& material) const
{
    if (material.Style == MaterialStyle::Phong)
    {
        return mPhongProgram;
    }
    else if (material.Style == MaterialStyle::Debug)
    {
        return mDebugProgram;
    }
    else
    {
        throw std::logic_error("No program for this material.");
    }
}

} // end namespace ng
