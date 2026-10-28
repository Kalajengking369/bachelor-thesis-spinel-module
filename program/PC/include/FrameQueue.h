#pragma once
#include "IFrameQueue.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

class FrameQueue : public IFrameQueue {
  private:
    std::queue<RxBuffer> queue;
    std::mutex mtx;
    std::condition_variable cv;
    size_t maxSize;
    bool stopped = false;

  public:
    FrameQueue(size_t maxLen = 1);
    ~FrameQueue() override = default;

    bool pop(RxBuffer& outFrame, uint32_t timeoutMs = 0xFFFFFFFF) override;
    bool push(const RxBuffer& frame) override;
    void stop();
};