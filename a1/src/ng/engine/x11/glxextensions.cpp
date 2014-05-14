#include "ng/engine/x11/glxextensions.hpp"

#include <cstring>

namespace ng
{

bool IsExtensionSupported(const char *extList, const char *extension)
{
    const char *start;
    const char *where, *terminator;

    /* Extension names should not have spaces. */
    where = std::strchr(extension, ' ');
    if (where || *extension == '\0')
        return false;

    size_t extlen = std::strlen(extension);
    /* It takes a bit of care to be fool-proof about parsing the
         OpenGL extensions string. Don't be fooled by sub-strings,
         etc. */
    for (start=extList;;) {
        where = std::strstr(start, extension);

        if (!where)
            break;

        terminator = where + extlen;

        if ( where == start || *(where - 1) == ' ' )
            if ( *terminator == ' ' || *terminator == '\0' )
                return true;

        start = terminator;
    }

    return false;
}

} // end namespace ng
