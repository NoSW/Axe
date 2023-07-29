#pragma once

#include <atomic>

namespace axe::thread
{

class SpinLock
{
public:
    void lock() noexcept
    {
        while (_mLock.test_and_set(std::memory_order_acquire)) {}
    }

    void unlock() noexcept { _mLock.clear(std::memory_order_release); }

private:
    std::atomic_flag _mLock;
};

}  // namespace axe::thread