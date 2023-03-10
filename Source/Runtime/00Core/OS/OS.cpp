#include "00Core/OS/OS.hpp"

#ifdef _WIN32
#include "processthreadsapi.h"
#include <codecvt>
#include <locale>
#elif defined(__APPLE__)
#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif
#elif defined(__wasm__)
#include <emscripten/threading.h>
#else
#include <sched.h>
#endif

namespace axe::os {

i32 get_core_id () noexcept{
    i32 core_ = -1;
#ifdef _WIN32
  core_ = GetCurrentProcessorNumber();
#elif defined(__APPLE__)
#if defined(__x86_64__) || defined(__i386__)
  uint32_t CPUInfo[4];
  __cpuid_count(1, 0, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
  if ((CPUInfo[3] & (1 << 9)) != 0) {
    core_ = (unsigned)CPUInfo[1] >> 24;
  }
#endif
#elif defined(__wasm__)
  core_ = emscripten_num_logical_cores();
#elif defined(WITH_UE)
  // This is the simplest way to do it without writing platform dependent
  // code.
  const int32 ProcessorCoreCount = FGenericPlatformMisc::NumberOfCoresIncludingHyperthreads();
  if (ProcessorCoreCount == 0) {
   ORT_THROW("Fatal error: 0 count processors from GetLogicalProcessorInformation");
  }
  core_ = ProcessorCoreCount;
#else
  core_ = sched_getcpu();
#endif
    return core_;
}

}