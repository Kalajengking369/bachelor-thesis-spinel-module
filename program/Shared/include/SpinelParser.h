#pragma once
#include "SpinelBuffers.h"

#include <cstdint>

class SpinelParser
{
private:
  enum class RxState
  {
    WAIT_PRE,
    WAIT_FRM,
    WAIT_NUM_1,
    WAIT_NUM_2,
    WAIT_SHORT_FRAME_ADDR,
    WAIT_BIN_SDATA,
    WAIT_ASCII_SDATA,
    WAIT_FOR_CR_FINALIZE,
    WAIT_FOR_CR_IGNORE,
    SUB_NUM
  };

  RxState rxState = RxState::WAIT_PRE;
  RxBuffer rxBuffer;

  uint16_t expectedLen = 0;
  uint8_t currentSum = 0;
  volatile uint8_t commErrors = 0;
  bool validFormat = false;

  void incError();
  bool isReceiving() const;

public:
  bool processByte(uint8_t byte);
  void handleTimeout();
  void resetParser();

  volatile uint8_t &getErrorCount();
  const RxBuffer &getParsedFrame() const;
};