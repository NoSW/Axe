#pragma once

#include <atomic>
#include <thread>

// @DISCUSSION@:
// memory_order_acquire: - no rw in the cur_thread can be reordered before this load;
//                       - all writes in other threads that release the same atomic variable are visible in the current thread
// memory_order_release: - no rw in the cur_thread can be reordered after this store
//                       - all writes in the current thread are visible in other threads that acquire the same atomic variable
// memory_order_acq_rel: a+b
// memory_order_seq_cst: plus a single total order exists in which all threads observe all modifications in the same order
namespace axe::thread
{

// mainly used for small critical section
class SpinLock
{
public:
    void lock() noexcept
    {
        while (_mLock.test_and_set(std::memory_order_acquire))
        {
            std::this_thread::yield();  // a hint to the impl to reschedule the execution of threads, allowing other threads to run.
        }
    }

    [[nodiscard]] bool tryLock() noexcept { return !_mLock.test_and_set(std::memory_order_acquire); }

    void unlock() noexcept { _mLock.clear(std::memory_order_release); }

private:
    std::atomic_flag _mLock{};
};

}  // namespace axe::thread