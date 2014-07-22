#ifndef NG_MD5LOADER_HPP
#define NG_MD5LOADER_HPP

#include <string>

namespace ng
{

class MD5Model;
class IReadFile;

bool TryLoadMD5Mesh(
        MD5Model& model,
        IReadFile& md5meshFile,
        std::string& error);

void LoadMD5Mesh(
        MD5Model& model,
        IReadFile& md5meshFile);

} // end namespace ng

#endif // NG_MD5LOADER_HPP
