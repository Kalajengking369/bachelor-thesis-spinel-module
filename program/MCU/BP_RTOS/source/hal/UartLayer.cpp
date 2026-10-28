#include "UartLayer.h"
#include "clock_config.h"
#include "fsl_reset.h"

UartLayer::UartLayer() : currentBaudRate(0)
{
}

bool UartLayer::init(uint32_t baudRate)
{
    // Select clock source for FLEXCOMM1 used by LPUART1.
    CLOCK_SetClkDiv(kCLOCK_DivFlexcom1Clk, 1u);
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM1);

    uint32_t uartFreq = CLOCK_GetLPFlexCommClkFreq(1U);

    lpuart_config_t uartConfig;
    LPUART_GetDefaultConfig(&uartConfig);
    uartConfig.baudRate_Bps = baudRate;
    uartConfig.enableTx = true;
    uartConfig.enableRx = true;
    RESET_ClearPeripheralReset(kFC1_RST_SHIFT_RSTn);
    if (LPUART_Init(LPUART1, &uartConfig, uartFreq) != kStatus_Success)
    {
        return false;
    }
    currentBaudRate = baudRate;
    // Enable RTS control for RS-485 transmitter direction.
    LPUART1->MODIR |= LPUART_MODIR_TXRTSE(1) | LPUART_MODIR_TXRTSPOL(1);

    LPUART_EnableInterrupts(LPUART1, kLPUART_RxDataRegFullInterruptEnable |
                                         kLPUART_RxOverrunInterruptEnable |
                                         kLPUART_FramingErrorInterruptEnable |
                                         kLPUART_NoiseErrorInterruptEnable);

    NVIC_SetPriority(LP_FLEXCOMM1_IRQn, 5);
    EnableIRQ(LP_FLEXCOMM1_IRQn);
    return true;
}

void UartLayer::sendData(const uint8_t *data, size_t length)
{
    if (data != nullptr && length > 0)
    {
        LPUART_WriteBlocking(LPUART1, data, length);
    }
}

bool UartLayer::setSpeed(uint32_t baudRate)
{
    // Change baud rate only after all queued data has been transmitted.
    while (!(LPUART_GetStatusFlags(LPUART1) & kLPUART_TransmissionCompleteFlag))
    {
    }

    // Clear received bytes before switching speed.
    uint8_t dummy;
    while (LPUART_GetStatusFlags(LPUART1) & kLPUART_RxDataRegFullFlag)
    {
        dummy = LPUART1->DATA;
        (void)dummy;
    }

    uint32_t uartFreq = CLOCK_GetLPFlexCommClkFreq(1U);
    if (LPUART_SetBaudRate(LPUART1, baudRate, uartFreq) == kStatus_Success)
    {
        currentBaudRate = baudRate;
        return true;
    }
    return false;
}

const uint32_t *UartLayer::getBaudRatePtr() const
{
    return &currentBaudRate;
}