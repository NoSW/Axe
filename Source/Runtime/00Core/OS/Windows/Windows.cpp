#if _WIN32
#include "00Core/OS/OS.hpp"

#include <Windows.h>
#include <codecvt>
#include <locale>

#include <processthreadsapi.h>

namespace axe::os
{

i32 get_current_core_id() noexcept { return GetCurrentProcessorNumber(); }

void set_current_thread_name(std::wstring_view name) noexcept
{
    SetThreadDescription(GetCurrentThread(), name.data());
}

void set_thread_name(void* pThreadNativeHandle, std::wstring_view name) noexcept
{
    SetThreadDescription(pThreadNativeHandle, name.data());
}

}  // namespace axe::os
#endif