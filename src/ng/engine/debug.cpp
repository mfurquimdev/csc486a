#include "ng/engine/util/debug.hpp"

#include <cstdarg>
#include <mutex>
#include <cstdio>

namespace ng
{

int DebugPrintf(const char* fmt, ...)
{
    static std::mutex sDebugPrintMutex;
    std::lock_guard<std::mutex> lock(sDebugPrintMutex);

    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);

    fflush(stdout);

    return ret;
}

} // end namespace ng
