#pragma once
#include "IUartLayer.h"

#include <cstdint>
#include <string>

class UartLayer : public IUartLayer {
  private:
    int fd = -1;
    uint32_t currentBaudRate = 9600;

  public:
    UartLayer(const std::string& portName, uint32_t baudRate = 9600);
    ~UartLayer() override;

    bool readByte(uint8_t& outByte, int timeoutMs = 0);
    void sendData(const uint8_t* data, size_t length) override;
    bool setSpeed(uint32_t baudRate) override;

    const uint32_t* getBaudRatePtr() const override;
};