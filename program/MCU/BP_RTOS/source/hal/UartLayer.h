#pragma once
#include "IUartLayer.h"
#include "fsl_lpuart.h"

class UartLayer : public IUartLayer
{
private:
    uint32_t currentBaudRate;

public:
    UartLayer();
    ~UartLayer() override = default;

    bool init(uint32_t baudRate);

    void sendData(const uint8_t *data, size_t length) override;
    bool setSpeed(uint32_t baudRate) override;
    const uint32_t *getBaudRatePtr() const override;
};