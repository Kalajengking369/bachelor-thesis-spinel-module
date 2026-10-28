#include "FrameQueue.h"

FrameQueue::FrameQueue(size_t maxLen) : maxSize(maxLen) {}

bool FrameQueue::pop(RxBuffer& outFrame, uint32_t timeoutMs) {
    std::unique_lock<std::mutex> lock(mtx);

    // Special timeout value means wait until a frame arrives or queue is stopped.
    if(timeoutMs == 0xFFFFFFFF) {
        cv.wait(lock, [this]() { return !queue.empty() || stopped; });
        if(stopped)
            return false;
    } else {
        bool result = cv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                  [this]() { return !queue.empty() || stopped; });
        if(!result || stopped) {
            return false;
        }
    }

    outFrame = queue.front();
    queue.pop();

    return true;
}

bool FrameQueue::push(const RxBuffer& frame) {
    std::lock_guard<std::mutex> lock(mtx);

    if(queue.size() >= maxSize) {
        return false;
    }

    queue.push(frame);
    cv.notify_one();

    return true;
}

void FrameQueue::stop() {
    std::lock_guard<std::mutex> lock(mtx);
    stopped = true;
    cv.notify_all();
}