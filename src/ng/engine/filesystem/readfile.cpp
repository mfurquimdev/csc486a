#include "ng/engine/filesystem/readfile.hpp"

namespace ng
{

bool getline(std::string& s, IReadFile& file)
{
    s = "";
    char ch;

    while (!file.EoF() && file.ReadRecords(&ch, sizeof(ch), 1) != 0)
    {
        if (ch == '\n')
        {
            break;
        }

        s += ch;
    }

    return !file.EoF();
}

} // end namespace ng
