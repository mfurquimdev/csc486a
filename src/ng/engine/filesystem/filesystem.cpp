#include "ng/engine/filesystem/filesystem.hpp"
#include "ng/engine/filesystem/readfile.hpp"

#include <cstdio>

namespace ng
{

class StdCReadFile : public IReadFile
{
    FILE* mFilePtr = nullptr;

public:
    StdCReadFile(const char* path, const char* mode)
    {
        mFilePtr = std::fopen(path, mode);
        if (mFilePtr == NULL)
        {
            throw std::runtime_error(std::string("Failed to open ") + path);
        }
    }

    ~StdCReadFile()
    {
        std::fclose(mFilePtr);
    }

    std::size_t ReadRecords(
        void *buffer,
        std::size_t recordSize,
        std::size_t recordCount) override
    {
        return std::fread(buffer, recordSize, recordCount, mFilePtr);
    }

    bool EoF() const override
    {
        return std::feof(mFilePtr);
    }
};

class StdCFileSystem : public IFileSystem
{
public:
    std::shared_ptr<IReadFile> GetReadFile(
        const char* path, FileReadMode mode) override
    {
        return std::make_shared<StdCReadFile>(
            path,
            mode == FileReadMode::Binary ? "rb"
          : mode == FileReadMode::Text ? "r"
          : throw std::logic_error("Invalid FileReadMode"));
    }
};

std::shared_ptr<IFileSystem> CreateFileSystem()
{
    return std::make_shared<StdCFileSystem>();
}

} // end namespace ng
