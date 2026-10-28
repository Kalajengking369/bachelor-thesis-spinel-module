#include "FrameQueue.h"

FrameQueue::FrameQueue(size_t queueLength)
{
    queueHandle = xQueueCreate(queueLength, sizeof(RxBuffer));
}

FrameQueue::~FrameQueue()
{
    if (queueHandle != nullptr)
    {
        vQueueDelete(queueHandle);
    }
}

bool FrameQueue::pop(RxBuffer &outFrame, uint32_t timeoutMs)
{
    if (queueHandle == nullptr)
        return false;

    // Convert interface timeout value to FreeRTOS ticks.
    TickType_t ticksToWait = (timeoutMs == 0xFFFFFFFF) ? portMAX_DELAY : pdMS_TO_TICKS(timeoutMs);

    return (xQueueReceive(queueHandle, &outFrame, ticksToWait) == pdTRUE);
}

bool FrameQueue::push(const RxBuffer &frame)
{
    if (queueHandle == nullptr)
        return false;

    return (xQueueSend(queueHandle, &frame, 0) == pdTRUE);
}

bool FrameQueue::pushFromISR(const RxBuffer &frame, BaseType_t *pxHigherPriorityTaskWoken)
{
    if (queueHandle == nullptr)
        return false;

    return (xQueueSendFromISR(queueHandle, &frame, pxHigherPriorityTaskWoken) == pdTRUE);
}