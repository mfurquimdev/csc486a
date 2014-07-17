#ifndef NG_READFILE_HPP
#define NG_READFILE_HPP

#include <cstddef>
#include <string>

namespace ng
{

class IReadFile
{
public:
    virtual ~IReadFile() = default;

    virtual std::size_t ReadRecords(
        void* buffer,
        std::size_t recordSize,
        std::size_t recordCount) = 0;

    virtual bool EoF() const = 0;
};

bool getline(std::string& s, IReadFile& file);

} // end namespace ng

#endif // NG_READFILE_HPP
