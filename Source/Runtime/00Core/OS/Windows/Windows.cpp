#if _WIN32
#include "00Core/OS/OS.hpp"

#include <Windows.h>
#include <codecvt>
#include <locale>

namespace axe::os
{

i32 get_current_core_id() noexcept { return GetCurrentProcessorNumber(); }

}  // namespace axe::os
#endif