// ----------------------------------------------------------------------------
// Overrides for the new and delete operations with DefaultMemoryResource
//
// See /Axe/Source/ThirdParty/mimalloc/include/mimalloc-new-delete.h,
// and <https://en.cppreference.com/w/cpp/memory/new/operator_new>
// and https://github.com/mtrebi/memory-allocators
// ---------------------------------------------------------------------------
#include "00Core/Memory/Memory.hpp"
#include <mimalloc.h>

#include <atomic>
#include <memory_resource>
#include <set>
#include <new>
#include <assert.h>

namespace axe::memory
{

#define AXE_MEM_DEBUG_SINGLE_THREAD (AXE_CORE_MEM_DEBUG_ENABLE & 1)

#if AXE_MEM_DEBUG_SINGLE_THREAD
struct DebugPointerPool
{
    static constexpr size_t POOL_SIZE = 1024 * 1024;  // 1MB, support count: ~10k active pointers
    DebugPointerPool() : data(mi_new_aligned(POOL_SIZE, 16)) {}
    ~DebugPointerPool() { mi_free_aligned(data, 16); }
    void* const data;
};

static DebugPointerPool gs_DebugPool;

// set upstream=nullptr to detect the case, in which max active pointer > supported count
static std::pmr::monotonic_buffer_resource gs_DebugMemoryResource(gs_DebugPool.data, DebugPointerPool::POOL_SIZE, nullptr);
static std::pmr::set<u64> gs_ActivePointers(&gs_DebugMemoryResource);
#endif

// ----------------------------------------------------------------------------
// We can manage default, namely global alloc in class DefaultMemoryResource
// ---------------------------------------------------------------------------
class DefaultMemoryResource final : public std::pmr::memory_resource
{
    // For covering std::pmr::polymorphic_allocator
    virtual void* do_allocate(size_t bytes, size_t align) override
    {
#if AXE_CORE_MEM_DEBUG_ENABLE
        _mAllocByPmrBytes += bytes;
        _mAllocByPmrCount++;
#endif
        return new_aligned(bytes, align);
    }
    virtual void do_deallocate(void* ptr, size_t bytes, size_t align) override
    {
#if AXE_CORE_MEM_DEBUG_ENABLE
        _mFreeByPmrBytes += bytes;
        _mFreeByPmrCount++;
#endif
        free_size_aligned(ptr, bytes, align);
    }
    virtual bool do_is_equal(const memory_resource& that) const noexcept override { return this == &that; }

    DefaultMemoryResource() noexcept
    {
        std::pmr::set_default_resource(this);
    }
    ~DefaultMemoryResource() noexcept
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

        if (totalAllocCount != totalFreeCount || pmrAllocCount != pmrFreeCount || pmrBytes != _mFreeByPmrBytes.load()) { printf("\033[41mERROR: memory leak!\033[0m\n"); }
#if AXE_MEM_DEBUG_SINGLE_THREAD
        for (const u64& p : gs_ActivePointers)
        {
            assert((u64)&p > (u64)gs_DebugPool.data && (u64)&p <= (u64)gs_DebugPool.data + DebugPointerPool::POOL_SIZE);
            printf("0x%llx -> ", p);
            printf("0x%04x", (u16)((p >> 48) & 0xffff));
            printf("'%04x", (u16)((p >> 32) & 0xffff));
            printf("'%04x", (u16)((p >> 16) & 0xffff));
            printf("'%04x\n", (u16)((p >> 0) & 0xffff));
        }
#endif
#endif
        std::pmr::set_default_resource(nullptr);
    }

#if AXE_CORE_MEM_DEBUG_ENABLE
private:
    std::atomic<uint32_t> _mAllocBytes      = 0;
    std::atomic<uint32_t> _mAllocCount      = 0;
    std::atomic<uint32_t> _mFreeCount       = 0;
    std::atomic<uint32_t> _mAllocByPmrBytes = 0;
    std::atomic<uint32_t> _mFreeByPmrBytes  = 0;
    std::atomic<uint32_t> _mAllocByPmrCount = 0;
    std::atomic<uint32_t> _mFreeByPmrCount  = 0;

public:
    [[nodiscard]] bool
    isLeak() const noexcept
    {
        return _mAllocCount == _mFreeCount;
    }
    [[nodiscard]] uint32_t getAllocCount() const noexcept { return _mAllocCount; }
    [[nodiscard]] uint32_t getAccumBytes() const noexcept { return _mAllocBytes; }
    [[nodiscard]] uint32_t getAccumByPmrBytes() const noexcept { return _mAllocByPmrBytes; }
    [[nodiscard]] uint32_t getAllocByPmrCount() const noexcept { return _mAllocByPmrCount; }
#else
    [[nodiscard]] bool isLeak() const noexcept
    {
        return false;
    }
    [[nodiscard]] uint32_t getAllocCount() const noexcept { return 0; }
    [[nodiscard]] uint32_t getAccumBytes() const noexcept { return 0; }
    [[nodiscard]] uint32_t getAccumByPmrBytes() const noexcept { return 0; }
    [[nodiscard]] uint32_t getAllocByPmrCount() const noexcept { return 0; }
#endif
public:
    inline static DefaultMemoryResource* const get() noexcept
    {
        static DefaultMemoryResource ins;
        return &ins;
    }

    // used for user-allocate
    [[nodiscard]] inline void* new_n(size_t bytes)
    {
        void* p = mi_new(bytes);
#if AXE_CORE_MEM_DEBUG_ENABLE
        _mAllocBytes += bytes;
        _mAllocCount++;
#if AXE_MEM_DEBUG_SINGLE_THREAD
        gs_ActivePointers.emplace((u64)p);
#endif
#endif
        return p;
    }
    inline void free(void* p) noexcept
    {
#if AXE_CORE_MEM_DEBUG_ENABLE
        _mFreeCount++;
#if AXE_MEM_DEBUG_SINGLE_THREAD
        gs_ActivePointers.erase((u64)p);
#endif
#endif
        /* Add some tracking code here */
        mi_free(p);
    }
    [[nodiscard]] inline void* new_nothrow(size_t bytes) noexcept
    {
        void* p = mi_new_nothrow(bytes);
#if AXE_CORE_MEM_DEBUG_ENABLE
        _mAllocBytes += bytes;
        _mAllocCount++;
#if AXE_MEM_DEBUG_SINGLE_THREAD
        gs_ActivePointers.emplace((u64)p);
#endif
#endif
        return p;
    }
    inline void free_size(void* p, size_t bytes) noexcept
    {
#if AXE_CORE_MEM_DEBUG_ENABLE
        _mFreeCount++;
#if AXE_MEM_DEBUG_SINGLE_THREAD
        gs_ActivePointers.erase((u64)p);
#endif
#endif
        /* Add some tracking code here */
        mi_free_size(p, bytes);
    }
    inline void free_aligned(void* p, size_t align) noexcept
    {
#if AXE_CORE_MEM_DEBUG_ENABLE
        _mFreeCount++;
#if AXE_MEM_DEBUG_SINGLE_THREAD
        gs_ActivePointers.erase((u64)p);
#endif
#endif
        /* Add some tracking code here */
        mi_free_aligned(p, align);
    }
    inline void free_size_aligned(void* p, size_t bytes, size_t align) noexcept
    {
#if AXE_CORE_MEM_DEBUG_ENABLE
        _mFreeCount++;
#if AXE_MEM_DEBUG_SINGLE_THREAD
        gs_ActivePointers.erase((u64)p);
#endif
#endif
        /* Add some tracking code here */
        mi_free_size_aligned(p, bytes, align);
    }
    [[nodiscard]] inline void* new_aligned(size_t bytes, size_t align)
    {
        auto* p = mi_new_aligned(bytes, align);
#if AXE_CORE_MEM_DEBUG_ENABLE
        _mAllocBytes += bytes;
        _mAllocCount++;
#if AXE_MEM_DEBUG_SINGLE_THREAD
        gs_ActivePointers.emplace((u64)p);
#endif
#endif
        return p;
    }
    [[nodiscard]] inline void* new_aligned_nothrow(size_t bytes, size_t align) noexcept
    {
        auto* p = mi_new_aligned_nothrow(bytes, align);
#if AXE_CORE_MEM_DEBUG_ENABLE
        _mAllocBytes += bytes;
        _mAllocCount++;
#if AXE_MEM_DEBUG_SINGLE_THREAD
        gs_ActivePointers.emplace((u64)p);
#endif
#endif
        /* Add some tracking code here */
        return p;
    }
};

DefaultMemoryResource* get_default_allocator() noexcept
{
    return axe::memory::DefaultMemoryResource::get();
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
