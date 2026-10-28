#pragma once
#include <cstddef>
#include <cstdint>

class IUartLayer
{
public:
  virtual ~IUartLayer() = default;

  virtual void sendData(const uint8_t *data, size_t length) = 0;
  virtual bool setSpeed(uint32_t baudRate) = 0;
  virtual const uint32_t *getBaudRatePtr() const = 0;
};