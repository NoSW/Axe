#pragma once

#include "00Core/Config.hpp"
#include <readerwriterqueue/readerwriterqueue.h>
#include <concurrentqueue/concurrentqueue.h>

namespace axe::thread
{

// @DISCUSSION@:
// Before using lockfree structure, ask yourself:
//     Do you try your best to avoid sharing data between threads?
//     Is the performance of your code really being dragged down by locking? (for general use, it's plenty speedy enough)
//
// pros of lockfree is simple: fast and faster. However, its cons:
//    1. It's hard to test out enough, which means the code always looks right and works well in 99% cases but crashes in 1% cases of future
//    2. highly dependent on underlying ISA and hardware. May it works on your machine but not on others
//
// Why can be lockfree? In other words, how std::atomic<T> works? The answer is *memory barrier*. (https://preshing.com/20120710/memory-barriers-are-like-source-control-operations/)
//

template <typename T, size_t MAX_BLOCK_SIZE = 512>                   // never re-make wheels :-)
using QueueSPSC = moodycamel::ReaderWriterQueue<T, MAX_BLOCK_SIZE>;  // single producer single consumer

template <typename T, size_t MAX_BLOCK_SIZE = 512>
using BlockingQueueSPSC = moodycamel::BlockingReaderWriterQueue<T, MAX_BLOCK_SIZE>;

template <typename T>
using QueueMPMC = moodycamel::ConcurrentQueue<T, moodycamel::ConcurrentQueueDefaultTraits>;

}  // namespace axe::thread