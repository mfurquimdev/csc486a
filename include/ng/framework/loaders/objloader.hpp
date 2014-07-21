#ifndef NG_OBJLOADER_HPP
#define NG_OBJLOADER_HPP

#include <string>

namespace ng
{

class ObjModel;
class IReadFile;

bool TryLoadObj(
        ObjModel& model,
        IReadFile& objFile,
        std::string& error);

void LoadObj(
        ObjModel& model,
        IReadFile& objFile);

} // end namespace ng

#endif // NG_OBJLOADER_HPP
