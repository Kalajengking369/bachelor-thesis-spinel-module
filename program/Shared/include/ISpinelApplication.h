#pragma once
#include "SpinelBuffers.h"

class ISpinelApplication
{
public:
  virtual ~ISpinelApplication() = default;
  virtual TxBuffer onFrameReceived(const RxBuffer &rawRx, const UnwrappedSData &sdata,
                                   bool isAscii) = 0;
  virtual void onResponseSent(bool needReset) = 0;
};