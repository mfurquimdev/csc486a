#include "ng/framework/scenegraph/shaderprofile.hpp"

#include "ng/engine/rendering/renderer.hpp"
#include "ng/engine/rendering/shaderprogram.hpp"

namespace ng
{

void ShaderProfileFactory::BuildShaders(const std::shared_ptr<IRenderer>& renderer)
{
    static const char* ambient_vsrc =
            "#version 100\n"

            "struct Light {\n"
            "    mediump vec3 Color;\n"
            "};\n"

            "uniform Light uLight;\n"
            "uniform highp mat4 uProjection;\n"
            "uniform highp mat4 uModelView;\n"
            "uniform mediump vec3 uTint;\n"

            "attribute highp vec4 iPosition;\n"

            "varying mediump vec3 fColor;\n"

            "void main() {\n"
            "    fColor = uTint * uLight.Color;\n"
            "    gl_Position = uProjection * uModelView * iPosition;\n"
            "}\n";

    static const char* ambient_fsrc =
            "#version 100\n"

            "varying mediump vec3 fColor;\n"

            "void main() {\n"
            "    gl_FragColor = vec4(fColor,1.0);\n"
            "}\n";

    mAmbientProgram = renderer->CreateShaderProgram();
    mAmbientProgram->Init(std::shared_ptr<const char>(ambient_vsrc, [](const char*){}),
                          std::shared_ptr<const char>(ambient_fsrc, [](const char*){}),
                          true);

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
            "uniform mediump vec3 uTint;\n"

            "attribute highp vec4 iPosition;\n"
            "attribute mediump vec3 iNormal;\n"

            "varying mediump vec3 fColor;\n"

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

            "varying mediump vec3 fColor;\n"

            "void main() {\n"
            "    gl_FragColor = vec4(fColor, 1.0);\n"
            "}\n";

    mGouraudDiffuseProgram = renderer->CreateShaderProgram();
    mGouraudDiffuseProgram->Init(std::shared_ptr<const char>(gouraud_vsrc, [](const char*){}),
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

ShaderProfile ShaderProfileFactory::GetProfileForMaterial(const Material& material) const
{
    ShaderProfile profile;

    if (material.Style == MaterialStyle::Phong)
    {
        profile.AmbientProgram = mAmbientProgram;
        profile.DiffuseProgram = mGouraudDiffuseProgram;
    }
    else if (material.Style == MaterialStyle::Debug)
    {
        profile.AmbientProgram = mDebugProgram;
    }
    else
    {
        throw std::logic_error("No profile for this material.");
    }

    return profile;
}

} // end namespace ng
