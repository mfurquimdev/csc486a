#ifndef NG_SHADERPROFILE_HPP
#define NG_SHADERPROFILE_HPP

#include "ng/framework/scenegraph/material.hpp"

#include <memory>
#include <map>

namespace ng
{

class IRenderer;
class IShaderProgram;
class Material;

class ShaderProfile
{
    std::map<Material,std::shared_ptr<IShaderProgram>,MaterialLess> mMaterialToProgram;

public:
    void BuildShaders(const std::shared_ptr<IRenderer>& renderer);

    std::shared_ptr<IShaderProgram> GetProgramForMaterial(const Material& material) const;
};

} // end namespace ng

#endif // NG_SHADERPROFILE_HPP
