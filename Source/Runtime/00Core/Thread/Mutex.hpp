#pragma once

#ifdef _WIN32
// #include <winnt.h>
#endif

#include "00Core/Config.hpp"

namespace axe::thread
{

// typedef RTL_CRITICAL_SECTION CRITICAL_SECTION;

struct Mutex
{
#ifdef _WIN32
    //   CRITICAL_SECTION mHandle;
#endif
};

}  // namespace axe::thread