#pragma once
#include "SpinelBuffers.h"

#include <array>
#include <cstdint>

class SDataLayer
{
public:
  SDataLayer() = default;
  UnwrappedSData unwrapSData(SpinelBufferView sdata, bool isAscii);

  size_t wrapBin(std::array<uint8_t, 256> &buf, const uint8_t *payload, size_t len, uint8_t addr,
                 uint8_t sig, uint8_t inst);

  size_t wrapAscii(std::array<uint8_t, 256> &buf, const char *payload, size_t len, uint8_t addr,
                   const char *inst);
};