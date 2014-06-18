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

            "uniform lowp vec4 uTint;\n"

            "varying mediump vec3 fLightTint;\n"

            "void main() {\n"
            "    gl_FragColor = vec4(fLightTint,1.0); // * uTint;\n"
            "}\n";

    std::shared_ptr<IShaderProgram> phong = renderer->CreateShaderProgram();
    phong->Init(std::shared_ptr<const char>(phong_vsrc, [](const char*){}),
                std::shared_ptr<const char>(phong_fsrc, [](const char*){}),
                true);

    mMaterialToProgram.emplace(Material(MaterialStyle::Phong, MaterialQuality::Medium), std::move(phong));
}

std::shared_ptr<IShaderProgram> ShaderProfile::GetProgramForMaterial(const Material& material) const
{
    return mMaterialToProgram.at(material);
}

} // end namespace ng
