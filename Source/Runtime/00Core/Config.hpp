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
#include <memory_resource>

// DISCUSSION: constexpr v.s. inline constexpr, see https://quuxplusone.github.io/blog/2022/07/08/inline-constexpr/
// constexpr, internal linkage, one entity per TU(Translation Unit, namely, a .cpp file)
// inline constexpr, external linkage, one entity for the entire program

namespace axe
{

using i8                     = signed char;
using i16                    = short;
using i32                    = int;
using i64                    = long long;
using u8                     = unsigned char;
using u16                    = unsigned short;
using u32                    = unsigned int;
using u64                    = unsigned long long;

inline constexpr i8 I8_MIN   = (-127 - 1);
inline constexpr i8 I8_MAX   = 127;
inline constexpr i16 I16_MIN = (-32767 - 1);
inline constexpr i16 I16_MAX = 32767;
inline constexpr i32 I32_MIN = (-2147483647 - 1);
inline constexpr i32 I32_MAX = 2147483647;
inline constexpr i64 I64_MIN = (-9223372036854775807ll - 1);
inline constexpr i64 I64_MAX = 9223372036854775807ll;
inline constexpr u8 U8_MAX   = 0xffu;
inline constexpr u16 U16_MAX = 0xffffu;
inline constexpr u32 U32_MAX = 0xffffffffu;
inline constexpr u64 U64_MAX = 0xffffffffffffffffull;

template <typename T>
inline constexpr u32 element_num(T *p)
{
    return sizeof(*p) / sizeof(p[0]);
}

template <typename T>
inline constexpr u32 bits_count(T n)
{
    u8 ret = 0;
    while (n)
    {
        n &= (n - 1);
        ret++;
    }
    return ret;
}

inline constexpr char AXE_ENGINE_NAME[] = "Axe";

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

#undef max
#undef min

//////////////////////////////////////////////////////////////////////////////////////////////
//                             debug marker
//////////////////////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
#ifndef _DEBUG
#define _DEBUG
#endif
#endif

#ifdef DEBUG
#ifndef _DEBUG
#define _DEBUG
#endif
#endif


#ifdef _DEBUG
#define AXE_CORE_ENABLE_DEBUG 1
#else
#define AXE_CORE_ENABLE_DEBUG 0
#endif

#define AXE_CORE_IO_DEBUG_ENABLE      AXE_CORE_ENABLE_DEBUG
#define AXE_CORE_LOG_DEBUG_ENABLE     AXE_CORE_ENABLE_DEBUG
#define AXE_CORE_MATH_DEBUG_ENABLE    AXE_CORE_ENABLE_DEBUG
#define AXE_CORE_MEM_DEBUG_ENABLE     AXE_CORE_ENABLE_DEBUG
#define AXE_CORE_NET_DEBUG_ENABLE     AXE_CORE_ENABLE_DEBUG
#define AXE_CORE_PROF_DEBUG_ENABLE    AXE_CORE_ENABLE_DEBUG
#define AXE_CORE_REFLECT_DEBUG_ENABLE AXE_CORE_ENABLE_DEBUG
#define AXE_CORE_THREAD_DEBUG_ENABLE  AXE_CORE_ENABLE_DEBUG
#define AXE_CORE_WINDOW_DEBUG_ENABLE  AXE_CORE_ENABLE_DEBUG

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

#if AXE_BUILD_DLL
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
