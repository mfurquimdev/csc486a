#ifndef NG_OBJLOADER_HPP
#define NG_OBJLOADER_HPP

#include <string>

namespace ng
{

class ObjShape;
class IReadFile;

bool TryLoadObj(
        ObjShape& shape,
        IReadFile& objFile,
        std::string& error);

void LoadObj(
        ObjShape& shape,
        IReadFile& objFile);

} // end namespace ng

#endif // NG_OBJLOADER_HPP
