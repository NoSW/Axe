#pragma once
#include "00Core/Config.hpp"
#include <string>
#include <thread>

namespace axe::os
{

i32 get_current_core_id() noexcept;

u32 get_core_count() noexcept;

void set_current_thread_name(std::wstring_view name) noexcept;

void set_thread_name(void* pThreadNativeHandle, std::wstring_view name) noexcept;

}  // namespace axe::os
