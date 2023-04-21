#pragma once
#include "00Core/Config.hpp"
#include "00Core/OS/OS.hpp"

// clang-format off
#define SPDLOG_FUNCTION     __func__
#define SPDLOG_LEVEL_NAMES { "TRA", "DEV", "INF", "WAR", "ERR", "UTE", " OFF" }

#if AXE_CORE_LOG_DEBUG_ENABLE
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#include <spdlog/spdlog.h>

#if _WIN32
#define AXEVS_DEBUG(fmt, ...) do { OutputDebugStringA((LPCSTR)(std::format(fmt "\n", ##__VA_ARGS__).c_str())); } while(0); // output msg to debug window of Visual Studio 20xx
#else
#define AXEVS_DEBUG(fmt, ...)
#endif

#define AXE_TRACE(...)              SPDLOG_TRACE("] " __VA_ARGS__);     // used only for print full visibility of what is happening in application
#define AXE_DEBUG(...)              SPDLOG_DEBUG("] " __VA_ARGS__);     // used for print debug status and troubleshooting
#define AXE_INFO(...)               SPDLOG_INFO("] " __VA_ARGS__);      // used for indicating that enter/exit a certain state, etc.
#define AXE_WARN(...)               SPDLOG_WARN("] " __VA_ARGS__);      // used for print something unexpected happened that might disturb one of the processes.
#define AXE_ERROR(...)              SPDLOG_ERROR("] " __VA_ARGS__);     // used for recoverable errors(e.g., can't found file, etc.)
#define AXE_UTERR(...)              SPDLOG_CRITICAL("] " __VA_ARGS__);  // used for unit tests
#define AXE_ASSERT(expression, ...) assert(expression)                  // used for unrecoverable errors(e.g., nullptr, overflow, etc.)
#define AXE_CHECK(expression, ...)  { if (!(expression)) { SPDLOG_ERROR(":" #expression "] " __VA_ARGS__); } }

// clang-format on
namespace axe::log
{

static void init() noexcept { spdlog::set_pattern("%^[%T] [%l] [%P:%t] [%s:%#] [%!%v%$"); }  // COLOR_BEGIN [time] [level] [PID:TID] [filename:line] [func] value COLOR_END

static void exit() noexcept { spdlog::shutdown(); }

}  // namespace axe::log