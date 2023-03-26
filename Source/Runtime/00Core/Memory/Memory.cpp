// ----------------------------------------------------------------------------
// Overrides for the new and delete operations with DefaultMemoryResource
//
// See /Axe/Source/ThirdParty/mimalloc/include/mimalloc-new-delete.h,
// and <https://en.cppreference.com/w/cpp/memory/new/operator_new>
// and https://github.com/mtrebi/memory-allocators
// ---------------------------------------------------------------------------
#include "00Core/Memory/Memory.hpp"
#include "00Core/OS/OS.hpp"
#include <mimalloc.h>

#include <memory_resource>
#include <new>
#include <assert.h>

namespace axe::memory
{

void* DefaultMemoryResource::do_allocate(size_t bytes, size_t align)
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    _mAllocByPmrBytes += bytes;
    _mAllocByPmrCount++;
#endif
    return new_aligned(bytes, align);
}

void DefaultMemoryResource::do_deallocate(void* ptr, size_t bytes, size_t align)
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    _mFreeByPmrBytes += bytes;
    _mFreeByPmrCount++;
#endif
    free_size_aligned(ptr, bytes, align);
}

DefaultMemoryResource::~DefaultMemoryResource() noexcept
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    uint32_t totalAllocCount = _mAllocCount.load();
    uint32_t totalFreeCount  = _mFreeCount.load();
    uint32_t pmrAllocCount   = _mAllocByPmrCount.load();
    uint32_t pmrFreeCount    = _mFreeByPmrCount.load();
    uint32_t totalBytes      = _mAllocBytes.load();
    uint32_t pmrBytes        = _mAllocByPmrBytes.load();

    printf("all: alloc/free count %u/%u, accum bytes %u\n", totalAllocCount, totalFreeCount, totalBytes);
    printf("pmr: alloc/free count %u/%u, accum bytes %u\n", pmrAllocCount, pmrFreeCount, pmrBytes);
    printf("new: alloc/free count %u/%u, accum bytes %u\n", totalAllocCount - pmrAllocCount, totalFreeCount - pmrFreeCount, totalBytes - pmrBytes);

    if (totalAllocCount != totalFreeCount || pmrAllocCount != pmrFreeCount || pmrBytes != _mFreeByPmrBytes.load()) { printf("\033[41mERROR: memory leak! %u pointer(s) \033[0m\n", totalAllocCount - totalFreeCount); }

#endif
    std::pmr::set_default_resource(nullptr);
}

#if AXE_CORE_MEM_DEBUG_ENABLE
[[nodiscard]] bool DefaultMemoryResource::isLeak() const noexcept
{
    return _mAllocCount == _mFreeCount;
}
[[nodiscard]] u32 DefaultMemoryResource::getAllocCount() const noexcept { return _mAllocCount; }
[[nodiscard]] u32 DefaultMemoryResource::getAccumBytes() const noexcept { return _mAllocBytes; }
[[nodiscard]] u32 DefaultMemoryResource::getAccumByPmrBytes() const noexcept { return _mAllocByPmrBytes; }
[[nodiscard]] u32 DefaultMemoryResource::getAllocByPmrCount() const noexcept { return _mAllocByPmrCount; }
#else
[[nodiscard]] bool DefaultMemoryResource::isLeak() const noexcept
{
    return false;
}
[[nodiscard]] u32 DefaultMemoryResource::getAllocCount() const noexcept { return 0; }
[[nodiscard]] u32 DefaultMemoryResource::getAccumBytes() const noexcept { return 0; }
[[nodiscard]] u32 DefaultMemoryResource::getAccumByPmrBytes() const noexcept { return 0; }
[[nodiscard]] u32 DefaultMemoryResource::getAllocByPmrCount() const noexcept { return 0; }
#endif

// used for user-allocate
[[nodiscard]] void* DefaultMemoryResource::realloc(void* p, size_t newsize) noexcept
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    _mAllocBytes += newsize;
    _mAllocCount++;
    _mFreeCount++;
#endif
    return mi_realloc(p, newsize);
}

[[nodiscard]] void* DefaultMemoryResource::new_n(size_t bytes)
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    _mAllocBytes += bytes;
    _mAllocCount++;
#endif
    return mi_new(bytes);
}

[[nodiscard]] void* DefaultMemoryResource::new_nothrow(size_t bytes) noexcept
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    _mAllocBytes += bytes;
    _mAllocCount++;
#endif
    return mi_new_nothrow(bytes);
}

[[nodiscard]] void* DefaultMemoryResource::new_aligned(size_t bytes, size_t align)
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    _mAllocBytes += bytes;
    _mAllocCount++;
#endif
    return mi_new_aligned(bytes, align);
}

[[nodiscard]] void* DefaultMemoryResource::new_aligned_nothrow(size_t bytes, size_t align) noexcept
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    _mAllocBytes += bytes;
    _mAllocCount++;
#endif
    return mi_new_aligned_nothrow(bytes, align);
}

void DefaultMemoryResource::free(void* p) noexcept
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    _mFreeCount++;
#endif
    mi_free(p);
}

void DefaultMemoryResource::free_size(void* p, size_t bytes) noexcept
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    _mFreeCount++;
#endif
    mi_free_size(p, bytes);
}

void DefaultMemoryResource::free_aligned(void* p, size_t align) noexcept
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    _mFreeCount++;
#endif
    mi_free_aligned(p, align);
}

void DefaultMemoryResource::free_size_aligned(void* p, size_t bytes, size_t align) noexcept
{
#if AXE_CORE_MEM_DEBUG_ENABLE
    _mFreeCount++;
#endif
    mi_free_size_aligned(p, bytes, align);
}

}  // namespace axe::memory

#if _MSC_VER
#pragma warning(push)
#pragma warning(disable : 28251)  // Disable the SAL annotations of new/delete by MSC
#endif

void operator delete(void* p) noexcept
{
    axe::memory::DefaultMemoryResource::get()->free(p);
}
void operator delete[](void* p) noexcept { axe::memory::DefaultMemoryResource::get()->free(p); };
void operator delete(void* p, const std::nothrow_t&) noexcept { axe::memory::DefaultMemoryResource::get()->free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { axe::memory::DefaultMemoryResource::get()->free(p); }

[[nodiscard]] void* operator new(std::size_t n) noexcept(false) { return axe::memory::DefaultMemoryResource::get()->new_n(n); }
[[nodiscard]] void* operator new[](std::size_t n) noexcept(false) { return axe::memory::DefaultMemoryResource::get()->new_n(n); }

[[nodiscard]] void* operator new(std::size_t n, const std::nothrow_t& tag) noexcept
{
    (void)(tag);
    return axe::memory::DefaultMemoryResource::get()->new_nothrow(n);
}

[[nodiscard]] void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept
{
    (void)(tag);
    return axe::memory::DefaultMemoryResource::get()->new_nothrow(n);
}

void operator delete(void* p, std::size_t n) noexcept { axe::memory::DefaultMemoryResource::get()->free_size(p, n); };
void operator delete[](void* p, std::size_t n) noexcept { axe::memory::DefaultMemoryResource::get()->free_size(p, n); };
void operator delete(void* p, std::align_val_t al) noexcept { axe::memory::DefaultMemoryResource::get()->free_aligned(p, static_cast<size_t>(al)); }
void operator delete[](void* p, std::align_val_t al) noexcept { axe::memory::DefaultMemoryResource::get()->free_aligned(p, static_cast<size_t>(al)); }
void operator delete(void* p, std::size_t n, std::align_val_t al) noexcept { axe::memory::DefaultMemoryResource::get()->free_size_aligned(p, n, static_cast<size_t>(al)); }
void operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept { axe::memory::DefaultMemoryResource::get()->free_size_aligned(p, n, static_cast<size_t>(al)); }
void operator delete(void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept { axe::memory::DefaultMemoryResource::get()->free_aligned(p, static_cast<size_t>(al)); }
void operator delete[](void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept { axe::memory::DefaultMemoryResource::get()->free_aligned(p, static_cast<size_t>(al)); }
[[nodiscard]] void* operator new(std::size_t n, std::align_val_t al) noexcept(false) { return axe::memory::DefaultMemoryResource::get()->new_aligned(n, static_cast<size_t>(al)); }
[[nodiscard]] void* operator new[](std::size_t n, std::align_val_t al) noexcept(false) { return axe::memory::DefaultMemoryResource::get()->new_aligned(n, static_cast<size_t>(al)); }
[[nodiscard]] void* operator new(std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { return axe::memory::DefaultMemoryResource::get()->new_aligned_nothrow(n, static_cast<size_t>(al)); }
[[nodiscard]] void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { return axe::memory::DefaultMemoryResource::get()->new_aligned_nothrow(n, static_cast<size_t>(al)); }

#if _MSC_VER
#pragma warning(pop)
#endif