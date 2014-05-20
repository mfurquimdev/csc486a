#ifndef NG_SHADERPROGRAM_HPP
#define NG_SHADERPROGRAM_HPP

namespace ng
{

class IShaderProgram
{
public:
    virtual ~IShaderProgram() = default;

    virtual void Init(std::shared_ptr<const char> vertexShaderSource,
                      std::shared_ptr<const char> fragmentShaderSource) = 0;

    virtual bool GetStatus(std::string& errorMessage) const = 0;
};

} // end namespace ng

#endif // NG_SHADERPROGRAM_HPP
