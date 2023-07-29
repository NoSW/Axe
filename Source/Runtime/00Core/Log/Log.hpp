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

#define SPDLOG_HEADER_ONLY
#include <spdlog/spdlog.h>

#if _WIN32
#define AXEVS_DEBUG(fmt, ...) do { OutputDebugStringA((LPCSTR)(std::format(fmt "\n", ##__VA_ARGS__).c_str())); } while(0); // output msg to debug window of Visual Studio 20xx
#else
#define AXEVS_DEBUG(fmt, ...)
#endif

// logs
#define AXE_TRACE(...)              SPDLOG_TRACE("] " __VA_ARGS__);     // used only for print full visibility of what is happening in application
#define AXE_DEBUG(...)              SPDLOG_DEBUG("] " __VA_ARGS__);     // used for print debug status and troubleshooting
#define AXE_INFO(...)               SPDLOG_INFO("] " __VA_ARGS__);      // used for indicating that enter/exit a certain state, etc.
#define AXE_WARN(...)               SPDLOG_WARN("] " __VA_ARGS__);      // used for print something unexpected happened that might disturb one of the processes.
#define AXE_ERROR(...)              SPDLOG_ERROR("] " __VA_ARGS__);     // used for recoverable errors(e.g., can't found file, etc.)
#define AXE_UTERR(...)              SPDLOG_CRITICAL("] " __VA_ARGS__);  // used for unit tests

#define AXE_CHECK(expression, ...)  { if (!(expression)) { SPDLOG_ERROR(":" #expression "] " __VA_ARGS__); } }  // just for print error 

// a good program should never trigger it
#define AXE_ASSERT(expression, ...) assert(expression) // used for unrecoverable errors(e.g., nullptr, overflow, etc.)

// a good program maybe trigger it due to various platform or hardware issues.
// usually for checking third-party apis
#define AXE_FAILED(expression) ((expression) ? false : axe::log::internal::log_return_value(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::err, "] " #expression, true)) // return expression, and print error msg if false
#define AXE_SUCCEEDED(expression) ((expression) ? true : axe::log::internal::log_return_value(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::err, "] " #expression, false)) // return expression, and print error msg if false

// clang-format on

namespace axe::log
{

namespace internal
{
// a log interface that has return value, only for AXE_FAILED and AXE_SUCCEEDED
inline static bool log_return_value(spdlog::source_loc loc, spdlog::level::level_enum lvl, spdlog::string_view_t msg, bool ret) noexcept
{
    spdlog::default_logger_raw()->log(loc, lvl, msg);
    return ret;
}
}  // namespace internal

static void init() noexcept { spdlog::set_pattern("%^[%T] [%l] [%P:%t] [%s:%#] [%!%v%$"); }  // COLOR_BEGIN [time] [level] [PID:TID] [filename:line] [func] value COLOR_END

static void exit() noexcept { spdlog::shutdown(); }

}  // namespace axe::log