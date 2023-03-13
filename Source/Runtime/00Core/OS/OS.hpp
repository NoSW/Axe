#pragma once
#include "00Core/Config.hpp"
#include <string>

namespace axe::os
{

i32 get_current_core_id() noexcept;

u32 get_core_count() noexcept;

}  // namespace axe::os
