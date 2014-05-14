#include "ng/engine/x11/xerrorhandler.hpp"

#include <stdexcept>
#include <array>

namespace ng
{

int ngXErrorHandler(Display* dpy, XErrorEvent* error)
{
    std::array<char,256> errorTextBuf;
    errorTextBuf[0] = '\0';
    XGetErrorText(dpy, error->error_code, errorTextBuf.data(), errorTextBuf.size());
    std::string msg = "Error " + std::to_string(error->error_code)
            + " (" + errorTextBuf.data() + "): request "
            + std::to_string(error->request_code) + "." + std::to_string(error->minor_code);
    throw std::runtime_error(msg);
}

} // end namespace ng
