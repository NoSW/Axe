// ----------------------------------------------------------------------------
// Overrides for the new and delete operations with DefaultAllocator
//
// See /Axe/Source/ThirdParty/mimalloc/include/mimalloc-new-delete.h,
// and <https://en.cppreference.com/w/cpp/memory/new/operator_new>
// and https://github.com/mtrebi/memory-allocators
// ---------------------------------------------------------------------------
#include <mimalloc.h>

#include <00Core/Config.hpp>
#include <atomic>
#include <memory_resource>
#include <new>

namespace axe::memory
{
// ----------------------------------------------------------------------------
// We can manage default, namely global alloc in class DefaultAllocator
// ---------------------------------------------------------------------------
class DefaultAllocator final : public std::pmr::memory_resource
{
    // For covering std::pmr::polymorphic_allocator
    virtual void* do_allocate(size_t bytes, size_t align) override { return new_aligned(bytes, align); }
    virtual void do_deallocate(void* ptr, size_t bytes, size_t align) override { free_size_aligned(ptr, bytes, align); }
    virtual bool do_is_equal(const memory_resource& that) const noexcept override { return this == &that; }

    AXE_NON_COPYABLE(DefaultAllocator)

    DefaultAllocator() noexcept { std::pmr::set_default_resource(this); }
    ~DefaultAllocator() noexcept { std::pmr::set_default_resource(nullptr); }

#ifdef _DEBUG
    std::atomic<u32> _totalBytes;
#endif

public:
    inline static DefaultAllocator* const get() noexcept
    {
        static DefaultAllocator ins;
        return &ins;
    }

    // used for user-allocate
    inline void* new_n(size_t bytes)
    {
        /* Add some tracking code here */
        return mi_new(bytes);
    }
    inline void free(void* p) noexcept
    {
        /* Add some tracking code here */
        mi_free(p);
    }
    inline void* new_nothrow(size_t bytes) noexcept
    {
        /* Add some tracking code here */
        return mi_new_nothrow(bytes);
    }
    inline void free_size(void* p, size_t bytes) noexcept
    {
        /* Add some tracking code here */
        mi_free_size(p, bytes);
    }
    inline void free_aligned(void* p, size_t align) noexcept
    {
        /* Add some tracking code here */
        mi_free_aligned(p, align);
    }
    inline void free_size_aligned(void* p, size_t bytes, size_t align) noexcept
    {
        /* Add some tracking code here */
        mi_free_size_aligned(p, bytes, align);
    }
    inline void* new_aligned(size_t bytes, size_t align)
    {
        /* Add some tracking code here */
        return mi_new_aligned(bytes, align);
    }
    inline void* new_aligned_nothrow(size_t bytes, size_t align) noexcept
    {
        /* Add some tracking code here */
        return mi_new_aligned_nothrow(bytes, align);
    }
};

}  // namespace axe::memory

#if _MSC_VER
#pragma warning(push)
#pragma warning(disable : 28251)  // Disable the SAL annotations of new/delete by MSC
#endif

void operator delete(void* p) noexcept
{
    axe::memory::DefaultAllocator::get()->free(p);
}
void operator delete[](void* p) noexcept { axe::memory::DefaultAllocator::get()->free(p); };
void operator delete(void* p, const std::nothrow_t&) noexcept { axe::memory::DefaultAllocator::get()->free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { axe::memory::DefaultAllocator::get()->free(p); }

[[nodiscard]] void* operator new(std::size_t n) noexcept(false) { return axe::memory::DefaultAllocator::get()->new_n(n); }
[[nodiscard]] void* operator new[](std::size_t n) noexcept(false) { return axe::memory::DefaultAllocator::get()->new_n(n); }

[[nodiscard]] void* operator new(std::size_t n, const std::nothrow_t& tag) noexcept
{
    (void)(tag);
    return axe::memory::DefaultAllocator::get()->new_nothrow(n);
}

[[nodiscard]] void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept
{
    (void)(tag);
    return axe::memory::DefaultAllocator::get()->new_nothrow(n);
}

void operator delete(void* p, std::size_t n) noexcept { axe::memory::DefaultAllocator::get()->free_size(p, n); };
void operator delete[](void* p, std::size_t n) noexcept { axe::memory::DefaultAllocator::get()->free_size(p, n); };
void operator delete(void* p, std::align_val_t al) noexcept { axe::memory::DefaultAllocator::get()->free_aligned(p, static_cast<size_t>(al)); }
void operator delete[](void* p, std::align_val_t al) noexcept { axe::memory::DefaultAllocator::get()->free_aligned(p, static_cast<size_t>(al)); }
void operator delete(void* p, std::size_t n, std::align_val_t al) noexcept { axe::memory::DefaultAllocator::get()->free_size_aligned(p, n, static_cast<size_t>(al)); }
void operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept { axe::memory::DefaultAllocator::get()->free_size_aligned(p, n, static_cast<size_t>(al)); }
void operator delete(void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept { axe::memory::DefaultAllocator::get()->free_aligned(p, static_cast<size_t>(al)); }
void operator delete[](void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept { axe::memory::DefaultAllocator::get()->free_aligned(p, static_cast<size_t>(al)); }
[[nodiscard]] void* operator new(std::size_t n, std::align_val_t al) noexcept(false) { return axe::memory::DefaultAllocator::get()->new_aligned(n, static_cast<size_t>(al)); }
[[nodiscard]] void* operator new[](std::size_t n, std::align_val_t al) noexcept(false) { return axe::memory::DefaultAllocator::get()->new_aligned(n, static_cast<size_t>(al)); }
[[nodiscard]] void* operator new(std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { return axe::memory::DefaultAllocator::get()->new_aligned_nothrow(n, static_cast<size_t>(al)); }
[[nodiscard]] void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { return axe::memory::DefaultAllocator::get()->new_aligned_nothrow(n, static_cast<size_t>(al)); }

#if _MSC_VER
#pragma warning(pop)
#endif