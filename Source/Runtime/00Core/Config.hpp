/*
@DISCUSSION@: Difference between `#pragma once` and include guard (i.e., #ifndef ...)
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

// @DISCUSSION@: constexpr v.s. inline constexpr, see https://quuxplusone.github.io/blog/2022/07/08/inline-constexpr/
// constexpr, internal linkage, one entity per TU(Translation Unit, namely, a .cpp file)
// inline constexpr, external linkage, one entity for the entire program

//! @CONVENTION@: macros defined in Axe
//!     - all macros defined in Axe must start with AXE_
//!            - for avoiding name collision and easy to use
//!     - all macros defined in Axe must be upper snake case
//!           - like AXE_MY_MACRO
//!     - all macros defined in Axe must be defined in 0XLayer/Config.hpp except for temporary-and-dedicated macros defined in .cpp
//!         - for quick to find and easy to maintain
//!         - for provide a read-friendly overview

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
//                             un-windows
//////////////////////////////////////////////////////////////////////////////////////////////
#undef max
#undef min

//////////////////////////////////////////////////////////////////////////////////////////////
//                             debug marker
//////////////////////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
#ifndef _DEBUG
#define _DEBUG 1  // make sure global debug macro is always defined
#endif
#endif

#ifdef DEBUG
#ifndef _DEBUG
#define _DEBUG 1  // make sure global debug macro is always defined
#endif
#endif

#if _DEBUG
#define AXE_CORE_IO_DEBUG_ENABLE      1  // used for controlling module-specified debugging
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
//                            readable marker
//////////////////////////////////////////////////////////////////////////////////////////////
#define AXE_PRIVATE                   // empty definition without any side effects, class member function marker,
                                      // indicates layer internal public interface that can be accessed only from current layer
#define AXE_THREAD_SAFE               // empty definition without any side effects,  function marker indicates that it should and must be thread-safe
#define AXE_(enable, comment) enable  // used for comment on an active/inactive piece of code

#define AXE_NON_COPYABLE(CLASS)               \
    CLASS(const CLASS &)            = delete; \
    CLASS(CLASS &&)                 = delete; \
    CLASS &operator=(const CLASS &) = delete; \
    CLASS &operator=(CLASS &&)      = delete;

namespace axe
{

struct NonCopyableNonMovable
{
    constexpr NonCopyableNonMovable() noexcept                      = default;
    ~NonCopyableNonMovable() noexcept                               = default;

    NonCopyableNonMovable(NonCopyableNonMovable &&)                 = delete;
    NonCopyableNonMovable &operator=(NonCopyableNonMovable &&)      = delete;
    NonCopyableNonMovable(const NonCopyableNonMovable &)            = delete;
    NonCopyableNonMovable &operator=(const NonCopyableNonMovable &) = delete;
};

// an alias of raw pointer used to indicate that nullptr is allowed, (e.g., nullable<const char> pOptionalName = nullptr;)
// which means we need to write some code like `if (pOptionalName == nullptr) {...}`.
// Otherwise, it's not allowed to be nullptr by default,(e.g., const char *pRequiredName = nullptr;)
// which means need to do some checks like `AXE_ASSERT(pRequiredName != nullptr);`
template <typename T>
using nullable = T *;
}  // namespace axe

//////////////////////////////////////////////////////////////////////////////////////////////
//                             platform, os, arch, hardware
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
#ifdef _WIN32
#define AXE_OS_WINDOWS 1
#elif defined(__linux__)
#define AXE_OS_LINUX 1
#else
#error "unsupported platform"
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//                             compiler
//////////////////////////////////////////////////////////////////////////////////////////////

// https://stackoverflow.com/questions/172587/what-is-the-difference-between-g-and-gcc
// @DISCUSSION@ gcc v.s. g++ (clang v.s. clang++ is also same)
//     - gcc will compile: .c and .cpp files as C and C++ respectively.
//     - g++ will compile: .c and .cpp files but they will all be treated as C++ files.
//     - g++ will automatically links in the std C++ libraries (gcc does not do this).
//     - g++ have more predefined macros. (e.g., __cplusplus )

#ifdef _MSC_VER
#define AXE_COMPILER_MSVC 1
#define AXE_DLL_IMPORT    __declspec(dllimport)  // For example, class AXE_DLL_IMPORT X {};
#define AXE_DLL_EXPORT    __declspec(dllexport)
#define AXE_DLL_HIDDEN
#define AXE_NON_NULL
#define AXE_NULLABLE

#elif defined(__clang__) || defined(__GNUC__)

#define AXE_DLL_IMPORT __attribute__((visibility("default")))
#define AXE_DLL_EXPORT __attribute__((visibility("default")))
#define AXE_DLL_HIDDEN __attribute__((visibility("hidden")))

#ifdef __clang__
#define AXE_COMPILER_CLANG 1
#define AXE_NONNULL        _Nonnull
#define AXE_NULLABLE       _Nullable
#else
#define AXE_COMPILER_GCC 1
#define AXE_NONNULL
#define AXE_NULLABLE
#endif

#else
#error "Unknown compiler"
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//      compile-time configs (constants at compile times)
//////////////////////////////////////////////////////////////////////////////////////////////
#define CORE_CCT_EXAMPLE_DATA 0

//////////////////////////////////////////////////////////////////////////////////////////////
//  runtime-time configs (init at start time and some members can be changed at any time)
//////////////////////////////////////////////////////////////////////////////////////////////
namespace axe::core
{
class RuntimeConfig
{
public:
    u32 getA() const { return _mExampleDataA; }  // example attribute
    void setA(u32 a) { _mExampleDataA = a; }     // example attribute

public:
    u32 getB() const { return _mExampleDataB; }  // example attribute

private:  // can be changed at any time
    u32 _mExampleDataA = 0;

private:  // only set at start time
    u32 _mExampleDataB = 0;
};
}  // namespace axe::core

// TODO: un-destroyed buffer in VMA?
