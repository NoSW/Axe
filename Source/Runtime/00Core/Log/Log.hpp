#pragma once
#include "00Core/Config.hpp"

// clang-format off
#define SPDLOG_FUNCTION     __func__
#define SPDLOG_LEVEL_NAMES { "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR", "UTERR", "OFF  " }

#if AXE_CORE_LOG_DEBUG_ENABLE
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#include <spdlog/spdlog.h>

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

static void init() noexcept { spdlog::set_pattern("[%T] [%^%l%$] [%P:%t] [%s:%#] [%!%v"); }  // [time] [level] [PID:TID] [filename:line] [func] value

static void exit() noexcept { spdlog::shutdown(); }

}  // namespace axe::log