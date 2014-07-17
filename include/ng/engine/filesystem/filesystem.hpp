#ifndef NG_FILESYSTEM_HPP
#define NG_FILESYSTEM_HPP

#include <memory>

namespace ng
{

class IReadFile;

enum class FileReadMode
{
    Text,
    Binary
};

class IFileSystem
{
public:
    virtual ~IFileSystem() = default;

    virtual std::shared_ptr<IReadFile> GetReadFile(
        const char* path, FileReadMode mode) = 0;
};

std::shared_ptr<IFileSystem> CreateFileSystem();

} // end namespace ng

#endif // NG_FILESYSTEM_HPP
