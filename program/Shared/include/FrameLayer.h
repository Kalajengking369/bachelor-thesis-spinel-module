#pragma once
#include "SpinelBuffers.h"

#include <array>
#include <cstdint>

class FrameLayer
{
private:
  uint8_t calcSum(const uint8_t *data, size_t len);

public:
  UnwrappedFrame unwrapFrame(SpinelBufferView frame);

  size_t wrapBin(std::array<uint8_t, 256> &buf, size_t endIdx);
  size_t wrapAscii(std::array<uint8_t, 256> &buf, size_t endIdx);
};