#include <pthread.h>

namespace ng
{

// sigh.
// https://bugs.launchpad.net/ubuntu/+source/nvidia-graphics-drivers-319/+bug/1248642
static void ForcePosixThreadsLink()
{
    pthread_getconcurrency();
}

} // end namespace ng
