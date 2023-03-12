#if __linux__
#include "00Core/OS/OS.hpp"
#include <sched.h>

namespace axe::os
{

i32 get_current_core_id() noexcept { return sched_getcpu(); }

}  // namespace axe::os
#endif