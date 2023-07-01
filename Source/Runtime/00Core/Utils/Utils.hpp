#pragma once

#include "00Core/Utils/Concepts.hpp"
#include <memory>

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
//                             memory
//////////////////////////////////////////////////////////////////////////////////////////////
namespace axe::utils
{

// copy plain old data from src to dst with specified element count, and check whether overlap is happened
template <T_POD POD>
inline void pod_memcpy(POD* dst, const POD* src, const size_t element_count) noexcept
{
    AXE_ASSERT(dst != nullptr);
    AXE_ASSERT(src != nullptr);
    AXE_ASSERT(dst >= src + element_count || dst + element_count <= src);  // avoid overlap
    memcpy(dst, src, element_count * sizeof(POD));
}

// copy plain old data from [begin, end) to dst, and check whether overlap is happened
template <T_POD POD>
inline void pod_memcpy(POD* dst, const POD* begin, const POD* end) noexcept
{
    AXE_ASSERT(begin != nullptr);
    AXE_ASSERT(end != nullptr);
    AXE_ASSERT(dst != nullptr);
    AXE_ASSERT(end >= begin);
    AXE_ASSERT(dst >= end || dst + (end - begin) <= begin);  // avoid overlap
    memcpy(dst, begin, (end - begin) * sizeof(POD));
}

// copy plain old data from src to dst with specified element count, and allow overlap to happen
template <T_POD POD>
inline void pod_memmove(POD* dst, const POD* src, const size_t element_count) noexcept
{
    AXE_ASSERT(dst != nullptr);
    AXE_ASSERT(src != nullptr);
    memmove(dst, src, element_count * sizeof(POD));
}

// copy plain old data from [begin, end) to dst, and allow overlap to happen
template <T_POD POD>
inline void pod_memmove(POD* dst, const POD* begin, const POD* end) noexcept
{
    AXE_ASSERT(begin != nullptr);
    AXE_ASSERT(end != nullptr);
    AXE_ASSERT(dst != nullptr);
    AXE_ASSERT(end >= begin);
    memmove(dst, begin, (end - begin) * sizeof(POD));
}

}  // namespace axe::utils
