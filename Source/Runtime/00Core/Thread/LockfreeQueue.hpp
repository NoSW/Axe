#pragma once

#include "00Core/Config.hpp"
#include <readerwriterqueue/readerwriterqueue.h>
#include <concurrentqueue/concurrentqueue.h>

namespace axe::thread
{

template <typename T, size_t MAX_BLOCK_SIZE = 512>                   // never re-make wheels :-)
using QueueSPSC = moodycamel::ReaderWriterQueue<T, MAX_BLOCK_SIZE>;  // single producer single consumer

template <typename T, size_t MAX_BLOCK_SIZE = 512>
using BlockingQueueSPSC = moodycamel::BlockingReaderWriterQueue<T, MAX_BLOCK_SIZE>;

template <typename T>
using QueueMPMC = moodycamel::ConcurrentQueue<T, moodycamel::ConcurrentQueueDefaultTraits>;

}  // namespace axe::thread