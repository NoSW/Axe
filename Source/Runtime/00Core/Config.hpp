/*
DISCUSSION: Difference between `#pragma once` and include guard (i.e., #ifndef ...)
see https://stackoverflow.com/questions/787533/is-pragma-once-a-safe-include-guard
    #pragma once
        - pros
            - Easier to use and reduces possibilities for bugs, e.g., bad names or name collisions
            - Faster compiler time theoretically (but quite negligible and will be overshadowed by the amount of time that
              actually building and linking the code would take)
        - cons
            - If you have the same file in different locations (we have this because our build system copies files around)
              then the compiler will think these are different files.
            - Not part of the standard, namely there is no standard behavior for #pragma once in diff compilers and
              no recourse if the compiler is erroneously culling an #include.
            - Incompatibility with older era compilers, e.g., before gcc3.4m or very specific embedded hardware
            - Difficult to make the test-cases implementation in case of mocks/test environment, due to circular etc dependencies
    #ifndef
        - pros
            - Always works for all compilers
        - cons
             - More complier time theoretically (but optimized in modern compiler)
             - Maintenance cost for guard names
*/
#pragma once
#include <chrono>
#include <limits>
#include <memory_resource>

// DISCUSSION: constexpr v.s. inline constexpr, see https://quuxplusone.github.io/blog/2022/07/08/inline-constexpr/
// constexpr, internal linkage, one entity per TU(Translation Unit, namely, a .cpp file)
// inline constexpr, external linkage, one entity for the entire program

namespace axe
{

using i8                           = std::int8_t;
using i16                          = std::int16_t;
using i32                          = std::int32_t;
using i64                          = std::int64_t;
using u8                           = std::uint8_t;
using u16                          = std::uint16_t;
using u32                          = std::uint32_t;
using u64                          = std::uint64_t;

inline constexpr i8 I8_MIN         = std::numeric_limits<i8>::min();
inline constexpr i8 I8_MAX         = std::numeric_limits<i8>::max();
inline constexpr i16 I16_MIN       = std::numeric_limits<i16>::min();
inline constexpr i16 I16_MAX       = std::numeric_limits<i16>::max();
inline constexpr i32 I32_MIN       = std::numeric_limits<i32>::min();
inline constexpr i32 I32_MAX       = std::numeric_limits<i32>::max();
inline constexpr i64 I64_MIN       = std::numeric_limits<i64>::min();
inline constexpr i64 I64_MAX       = std::numeric_limits<i64>::max();
inline constexpr u8 U8_MAX         = std::numeric_limits<u8>::max();
inline constexpr u16 U16_MAX       = std::numeric_limits<u16>::max();
inline constexpr u32 U32_MAX       = std::numeric_limits<u32>::max();
inline constexpr u64 U64_MAX       = std::numeric_limits<u64>::max();
inline constexpr float FLOAT_MIN   = std::numeric_limits<float>::min();
inline constexpr float FLOAT_MAX   = std::numeric_limits<float>::max();
inline constexpr double DOUBLE_MIN = std::numeric_limits<double>::min();
inline constexpr double DOUBLE_MAX = std::numeric_limits<double>::max();

}  // namespace axe

//////////////////////////////////////////////////////////////////////////////////////////////
//                             benchmark
//////////////////////////////////////////////////////////////////////////////////////////////
#define AXE_BENCHMARK(ITERATIONS, baseline, testFunc, ...)                                             \
    {                                                                                                  \
        const auto start = std::chrono::system_clock::now();                                           \
        for (u64 i = 0; i < ITERATIONS; ++i)                                                           \
        {                                                                                              \
            testFunc(__VA_ARGS__);                                                                     \
        }                                                                                              \
        const auto stop = std::chrono::system_clock::now();                                            \
        const auto secs = std::chrono::duration<double>(stop - start);                                 \
        const auto sec  = secs.count();                                                                \
        baseline        = baseline == 0.0 ? sec : baseline;                                            \
        std::cout << std::format("{:<20}: {} sec; speed up: {:.3}\n", #testFunc, sec, baseline / sec); \
    }

//////////////////////////////////////////////////////////////////////////////////////////////
//                             class helpers
//////////////////////////////////////////////////////////////////////////////////////////////
#define AXE_NON_COPYABLE(CLASS)                      \
    CLASS(const CLASS &&)                  = delete; \
    CLASS(const CLASS &)                   = delete; \
    const CLASS &operator=(const CLASS &)  = delete; \
    const CLASS &operator=(const CLASS &&) = delete;

//////////////////////////////////////////////////////////////////////////////////////////////
//                             misc
//////////////////////////////////////////////////////////////////////////////////////////////

// used for comment on an active/inactive piece of code
#define AXE_(enable, comment) enable

//////////////////////////////////////////////////////////////////////////////////////////////
//                             un-windows
//////////////////////////////////////////////////////////////////////////////////////////////
#undef max
#undef min

//////////////////////////////////////////////////////////////////////////////////////////////
//                             debug marker
//////////////////////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
#ifndef _DEBUG
#define _DEBUG 1
#endif
#endif

#ifdef DEBUG
#ifndef _DEBUG
#define _DEBUG 1
#endif
#endif

#if _DEBUG
#define AXE_CORE_IO_DEBUG_ENABLE      1
#define AXE_CORE_LOG_DEBUG_ENABLE     1
#define AXE_CORE_MATH_DEBUG_ENABLE    1
#define AXE_CORE_MEM_DEBUG_ENABLE     1
#define AXE_CORE_NET_DEBUG_ENABLE     1
#define AXE_CORE_PROF_DEBUG_ENABLE    1
#define AXE_CORE_REFLECT_DEBUG_ENABLE 1
#define AXE_CORE_THREAD_DEBUG_ENABLE  1
#define AXE_CORE_WINDOW_DEBUG_ENABLE  1
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//                            layer visibility marker
//////////////////////////////////////////////////////////////////////////////////////////////
#define AXE_PUBLIC  // interface that can be accessed from higher layer

//////////////////////////////////////////////////////////////////////////////////////////////
//                             platform, os, compiler, arch, hardware
//////////////////////////////////////////////////////////////////////////////////////////////
/*
 * There lots of macros to identify platform, compilers etc, which usually added by selected compiler.
 * To be more straightforward, Axe use the following macros generated by compilers instead of warping in-house that,
 * which work in most cases:  (for more compatible, see https://github.com/boostorg/predef)
 *   - using  `_MSC_VER`, `__clang__`, `__GNUC__` to check compilers
 *   - using  `_WIN32`, `__APPLE__` `__linux__` to check platforms(or OS)
 *   - using `__cplusplus > 201703L` to check support of c++20  (usually, it needs Clang>=10, GCC>=11, MSVC>=19)
 *   - using `_DEBUG` to check global build type
 */
#if !defined(_WIN32) && !defined(__linux__)
#error "unsupported platform"
#endif

#if AXE_(0, "not used yet")
#if _MSC_VER
#define AXE_API_EXPORT __declspec(dllexport)
#define AXE_API_IMPORT __declspec(dllimport)
#define AXE_API_LOCAL
#elif __clang__ || __GNUC__
#define AXE_API_EXPORT __attribute__((visibility("default")))
#define AXE_API_LOCAL  __attribute__((visibility("hidden")))
#define AXE_API_IMPORT
#else
#error "unsupported compiler"
#endif
#endif
