#pragma once
#include "FrameLayer.h"
#include "IFrameQueue.h"
#include "ISpinelApplication.h"
#include "IUartLayer.h"
#include "SDataLayer.h"
#include "SpinelBuffers.h"

#include <array>

class SpinelProtocolManager
{
private:
  IFrameQueue &rxQueue;
  IUartLayer &uart;
  ISpinelApplication &app;

  SDataLayer sdataLayer;
  FrameLayer frameLayer;

  void processFrame(const RxBuffer &rx);
  void sendResponse(uint8_t addr, uint8_t ackCode, uint8_t sig, const uint8_t *data, size_t len,
                    bool isAscii);

public:
  SpinelProtocolManager(IFrameQueue &queue, IUartLayer &uart, ISpinelApplication &app);
  void run();

  void sendRequest(uint8_t addr, uint8_t inst, uint8_t sig, const uint8_t *data = nullptr,
                   size_t len = 0);
  void sendRequest(char addr, const char *inst, const char *data = nullptr, size_t len = 0);
};
