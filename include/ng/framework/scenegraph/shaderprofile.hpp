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
    std::shared_ptr<IShaderProgram> mPhongProgram;
    std::shared_ptr<IShaderProgram> mDebugProgram;

public:
    void BuildShaders(const std::shared_ptr<IRenderer>& renderer);

    std::shared_ptr<IShaderProgram> GetProgramForMaterial(const Material& material) const;
};

} // end namespace ng

#endif // NG_SHADERPROFILE_HPP
