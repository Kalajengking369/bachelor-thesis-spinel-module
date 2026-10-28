#pragma once
#include "SpinelBuffers.h"

#include <cstdint>

class IFrameQueue
{
public:
  virtual ~IFrameQueue() = default;
  virtual bool pop(RxBuffer &outFrame, uint32_t timeoutMs = 0xFFFFFFFF) = 0;

  virtual bool push(const RxBuffer &frame) = 0;
};