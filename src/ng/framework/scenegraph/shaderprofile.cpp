#include "ng/framework/scenegraph/shaderprofile.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/shaderprogram.hpp"

namespace ng
{

void ShaderProfile::BuildShaders(const std::shared_ptr<IRenderer>& renderer)
{
    static const char* gouraud_vsrc =
            "#version 100\n"

            "struct Light {\n"
            "    highp vec3 Position;\n"
            "    highp float Radius;\n"
            "    mediump vec3 Color;\n"
            "};\n"

            "uniform Light uLight;\n"
            "uniform highp mat4 uProjection;\n"
            "uniform highp mat4 uModelView;\n"
            "uniform lowp vec3 uTint;\n"

            "attribute highp vec4 iPosition;\n"
            "attribute mediump vec3 iNormal;\n"

            "varying lowp vec3 fColor;\n"

            "void main() {\n"
            "    vec3 P = iPosition.xyz;\n"
            "    vec3 toLight = uLight.Position - P;\n"
            "    float distanceLengthRatio = length(toLight) / uLight.Radius;\n"
            "    float attenuation = max(0.0, 1.0 - (distanceLengthRatio * distanceLengthRatio));\n"
            "    vec3 L = normalize(toLight);\n"
            "    fColor = uTint * uLight.Color * attenuation * max(dot(iNormal, L),0.0);\n"
            "    gl_Position = uProjection * uModelView * iPosition;\n"
            "}\n";

    static const char* gouraud_fsrc =
            "#version 100\n"

            "varying lowp vec3 fColor;\n"

            "void main() {\n"
            "    gl_FragColor = vec4(fColor, 1.0);\n"
            "}\n";

    mPhongProgram = renderer->CreateShaderProgram();
    mPhongProgram->Init(std::shared_ptr<const char>(gouraud_vsrc, [](const char*){}),
                        std::shared_ptr<const char>(gouraud_fsrc, [](const char*){}),
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
