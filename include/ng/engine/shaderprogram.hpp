#ifndef NG_SHADERPROGRAM_HPP
#define NG_SHADERPROGRAM_HPP

namespace ng
{

class IShaderProgram
{
public:
    virtual ~IShaderProgram() = default;

    virtual void Init(std::shared_ptr<const char> vertexShaderSource,
                      std::shared_ptr<const char> fragmentShaderSource,
                      bool validate) = 0;

    virtual std::pair<bool,std::string> GetStatus() const = 0;
};

} // end namespace ng

#endif // NG_SHADERPROGRAM_HPP
