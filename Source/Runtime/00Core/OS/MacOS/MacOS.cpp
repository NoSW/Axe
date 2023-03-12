#if __APPLE__

#include "00Core/OS/OS.hpp"
// #if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
// #else
// #error "unsupported arch"
// #endifs
namespace axe::os
{

i32 get_current_core_id() noexcept
{
    u32 CPUInfo[4];
    __cpuid_count(1, 0, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    return ((CPUInfo[3] & (1 << 9)) != 0) ? (i32)((unsigned)CPUInfo[1] >> 24) : -1;
}

}  // namespace axe::os
#endif