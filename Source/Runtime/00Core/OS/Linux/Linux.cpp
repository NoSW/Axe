#if __linux__
#include "00Core/OS/OS.hpp"
#include <sched.h>

namespace axe::os
{

i32 get_current_core_id() noexcept { return sched_getcpu(); }

void set_current_thread_name(std::wstring_view name) noexcept {}

void set_thread_name(void* pThreadNativeHandle, std::wstring_view name) noexcept {}
}  // namespace axe::os
#endif