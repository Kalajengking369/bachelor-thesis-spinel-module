#pragma once
#include "IFrameQueue.h"
#include "FreeRTOS.h"
#include "queue.h"

class FrameQueue : public IFrameQueue
{
private:
    QueueHandle_t queueHandle;

public:
    FrameQueue(size_t len = 1);
    ~FrameQueue() override;

    bool pop(RxBuffer &outFrame, uint32_t timeoutMs = 0xFFFFFFFF) override;
    bool push(const RxBuffer &frame) override;
    bool pushFromISR(const RxBuffer &frame, BaseType_t *pxHigherPriorityTaskWoken);
};