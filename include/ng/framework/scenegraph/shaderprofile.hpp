#ifndef NG_SHADERPROFILE_HPP
#define NG_SHADERPROFILE_HPP

#include "ng/framework/scenegraph/material.hpp"

#include <memory>

namespace ng
{

class IRenderer;
class IShaderProgram;
class Material;

class ShaderProfile
{
public:
    std::shared_ptr<IShaderProgram> AmbientProgram;
    std::shared_ptr<IShaderProgram> DiffuseProgram;
};

class ShaderProfileFactory
{
    std::shared_ptr<IShaderProgram> mAmbientProgram;
    std::shared_ptr<IShaderProgram> mGouraudDiffuseProgram;
    std::shared_ptr<IShaderProgram> mDebugProgram;

public:
    void BuildShaders(const std::shared_ptr<IRenderer>& renderer);

    ShaderProfile GetProfileForMaterial(const Material& material) const;
};

} // end namespace ng

#endif // NG_SHADERPROFILE_HPP
