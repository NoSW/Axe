#include "00Core/OS/OS.hpp"

#include <thread>

namespace axe::os
{

/*
 * Considering Hyper-Threading(or Simultaneous Multi-Threading, SMT) -- two or more hardware threads on one core --,
 * the safest assumption is to have no more than one CPU-intensive thread per CPU core. Having more CPU-intensive threads
 * than CPU cores gives little or no benefits, and brings the extra overhead and complexity of additional threads.
 * refer to: https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/DxTechArts/coding-for-multiple-cores.md
 */

u32 get_core_count() noexcept { return std::thread::hardware_concurrency(); }

}  // namespace axe::os