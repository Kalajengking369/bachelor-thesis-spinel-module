/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    main.cpp
 * @brief   Application entry point.
 */

#include <hal/Ds1621.h>
#include "storage/ConfigMapper.h"
#include "app/FrameQueue.h"
#include "FreeRTOS.h"
#include "SpinelParser.h"
#include "SpinelProtocolManager.h"
#include "hal/UartLayer.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_clock.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_lpflexcomm.h"
#include "fsl_lpi2c_freertos.h"
#include "fsl_lpuart.h"
#include "fsl_port.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "task.h"
#include "timers.h"

#include "app/Application.h"

struct TemperatureTaskContext
{
    DeviceManager *devManager;
    lpi2c_rtos_handle_t *i2cHandle;
};

FrameQueue rxQueue(1);
SpinelParser parser;
TimerHandle_t rxTimeoutTimer = nullptr;
lpi2c_rtos_handle_t rtos_i2c_handle;

volatile uint8_t g_actives = 0;
volatile uint8_t g_checked = 0;
volatile uint32_t g_baud = 0;

static void InitTmp275I2c(void)
{
    CLOCK_EnableClock(kCLOCK_LPFlexComm2);
    CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 1u);
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);
    RESET_ClearPeripheralReset(kFC2_RST_SHIFT_RSTn);

    lpi2c_master_config_t config;
    LPI2C_MasterGetDefaultConfig(&config);
    config.baudRate_Hz = 100000U;

    g_baud = CLOCK_GetLPFlexCommClkFreq(2);
    NVIC_SetPriority(LP_FLEXCOMM2_IRQn, 5);
    LPI2C_RTOS_Init(&rtos_i2c_handle, LPI2C2, &config, g_baud);
}
/*
bool tmp275_Init(lpi2c_rtos_handle_t* handle, uint8_t addr) {
    uint8_t cfg = 0b01100000; // 12-bit resolution

    lpi2c_master_transfer_t xfer{};
    xfer.slaveAddress = addr;
    xfer.direction = kLPI2C_Write;
    xfer.subaddress = 0x01; // CONFIG register
    xfer.subaddressSize = 1;
    xfer.data = &cfg;
    xfer.dataSize = 1;
    xfer.flags = kLPI2C_TransferDefaultFlag;

    return (LPI2C_RTOS_Transfer(handle, &xfer) == kStatus_Success);
}
*/
static void TMP275Reset(void)
{
    // P4_0 = SDA, P4_1 = SCL
    PORT_SetPinMux(PORT4, 0U, kPORT_MuxAlt0);
    PORT_SetPinMux(PORT4, 1U, kPORT_MuxAlt0);

    gpio_pin_config_t outCfg = {kGPIO_DigitalOutput, 1};
    GPIO_PinInit(GPIO4, 0U, &outCfg);
    GPIO_PinInit(GPIO4, 1U, &outCfg);

    // drž SCL low > 54 ms
    GPIO_PinWrite(GPIO4, 1U, 0);
    SDK_DelayAtLeastUs(60000, SystemCoreClock);
    GPIO_PinWrite(GPIO4, 1U, 1);

    // vrátit piny zpět na FC2
    PORT_SetPinMux(PORT4, 0U, kPORT_MuxAlt2);
    PORT_SetPinMux(PORT4, 1U, kPORT_MuxAlt2);

    PORT4->PCR[0] |= PORT_PCR_ODE_MASK | PORT_PCR_PS_MASK | PORT_PCR_IBE_MASK;
    PORT4->PCR[1] |= PORT_PCR_ODE_MASK | PORT_PCR_PS_MASK | PORT_PCR_IBE_MASK;
}

static void I2C_BusReset(void)
{
    // Temporarily switch I2C pins to GPIO.
    PORT_SetPinMux(PORT4, 0U, kPORT_MuxAlt0);
    PORT_SetPinMux(PORT4, 1U, kPORT_MuxAlt0);

    gpio_pin_config_t outCfg = {kGPIO_DigitalOutput, 1};
    GPIO_PinInit(GPIO4, 0U, &outCfg);
    GPIO_PinInit(GPIO4, 1U, &outCfg);

    // Generate clock pulses to release a stuck I2C slave.
    for (int i = 0; i < 9; i++)
    {
        GPIO_PinWrite(GPIO4, 1U, 0);
        SDK_DelayAtLeastUs(5, SystemCoreClock);
        GPIO_PinWrite(GPIO4, 1U, 1);
        SDK_DelayAtLeastUs(5, SystemCoreClock);
    }

    GPIO_PinWrite(GPIO4, 0U, 0);
    SDK_DelayAtLeastUs(5, SystemCoreClock);
    GPIO_PinWrite(GPIO4, 1U, 1);
    SDK_DelayAtLeastUs(5, SystemCoreClock);
    GPIO_PinWrite(GPIO4, 0U, 1);
    SDK_DelayAtLeastUs(5, SystemCoreClock);

    // Return pins back to FLEXCOMM2 I2C function.
    PORT_SetPinMux(PORT4, 0U, kPORT_MuxAlt2);
    PORT_SetPinMux(PORT4, 1U, kPORT_MuxAlt2);

    PORT4->PCR[0] |= PORT_PCR_ODE_MASK | PORT_PCR_PS_MASK | PORT_PCR_IBE_MASK;
    PORT4->PCR[1] |= PORT_PCR_ODE_MASK | PORT_PCR_PS_MASK | PORT_PCR_IBE_MASK;
}

void DetectSensors(DeviceManager &devManager, lpi2c_rtos_handle_t *i2cHandle)
{
    auto &devs = devManager.getDevices();

    for (auto &dev : devs)
    {
        uint16_t raw = 0;
        int16_t temp32 = 0;

        if (ds1621_Init(i2cHandle, dev.i2cAddr))
        {
            // Wait for the first DS1621 conversion result.
            vTaskDelay(pdMS_TO_TICKS(1000));

            TempStatus status = ds1621_ReadTemperature(i2cHandle, dev.i2cAddr, raw, temp32);

            if (status == TempStatus::OK)
            {
                taskENTER_CRITICAL();
                dev.lastTempRaw = raw;
                dev.lastTemp32 = temp32;
                taskEXIT_CRITICAL();
            }
            else
            {
                taskENTER_CRITICAL();
                dev.isConnected = false;
                taskEXIT_CRITICAL();
            }
        }
        else
        {
            taskENTER_CRITICAL();
            dev.isConnected = false;
            taskEXIT_CRITICAL();
        }
    }
}

void RxTimeoutCallback(TimerHandle_t xTimer)
{
    taskENTER_CRITICAL();
    parser.handleTimeout();
    taskEXIT_CRITICAL();
}

extern "C" void LP_FLEXCOMM1_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t flags = LPUART_GetStatusFlags(LPUART1);

    if (flags & (kLPUART_RxOverrunFlag | kLPUART_FramingErrorFlag | kLPUART_NoiseErrorFlag))
    {
        // UART error invalidates the currently received frame.
        LPUART_ClearStatusFlags(LPUART1, kLPUART_RxOverrunFlag | kLPUART_FramingErrorFlag |
                                             kLPUART_NoiseErrorFlag);

        if (flags & kLPUART_RxDataRegFullFlag)
        {
            (void)LPUART_ReadByte(LPUART1);
        }

        if (rxTimeoutTimer != nullptr)
        {
            xTimerStopFromISR(rxTimeoutTimer, &xHigherPriorityTaskWoken);
        }
        parser.handleTimeout();

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        SDK_ISR_EXIT_BARRIER;
        return;
    }

    if (flags & kLPUART_RxDataRegFullFlag)
    {
        uint8_t byte = LPUART_ReadByte(LPUART1);

        if (parser.processByte(byte))
        {
            // Complete frame received, stop inter-byte timeout and queue it for processing.
            if (rxTimeoutTimer != nullptr)
            {
                xTimerStopFromISR(rxTimeoutTimer, &xHigherPriorityTaskWoken);
            }
            const RxBuffer &rx = parser.getParsedFrame();
            rxQueue.pushFromISR(rx, &xHigherPriorityTaskWoken);
        }
        else
        {
            // Restart inter-byte timeout while a frame is being received.
            if (rxTimeoutTimer != nullptr)
            {
                xTimerResetFromISR(rxTimeoutTimer, &xHigherPriorityTaskWoken);
            }
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    SDK_ISR_EXIT_BARRIER;
}

void SpinelManagerTask(void *pvParameters)
{
    SpinelProtocolManager *manager = static_cast<SpinelProtocolManager *>(pvParameters);
    manager->run();
    vTaskDelete(nullptr);
}

void TemperatureTask(void *pvParameters)
{

    auto *ctx = static_cast<TemperatureTaskContext *>(pvParameters);

    // Detect sensors once before starting periodic temperature updates.
    DetectSensors(*ctx->devManager, ctx->i2cHandle);

    vTaskDelay(pdMS_TO_TICKS(50));
    for (;;)
    {
        auto &devs = ctx->devManager->getDevices();
        for (auto &dev : devs)
        {
            if (!dev.isConnected)
                continue;

            uint16_t raw = 0;
            int16_t temp32 = 0;

            TempStatus status = ds1621_ReadTemperature(ctx->i2cHandle, dev.i2cAddr, raw, temp32);
            if (status == TempStatus::OK)
            {
                taskENTER_CRITICAL();
                dev.lastTempRaw = raw;
                dev.lastTemp32 = temp32;
                taskEXIT_CRITICAL();
            }
            else if (status == TempStatus::NAK)
            {
                taskENTER_CRITICAL();
                dev.isConnected = false;
                taskEXIT_CRITICAL();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    I2C_BusReset();
    // TMP275Reset();
    InitTmp275I2c();

    static ConfigStorage storage;
    if (!storage.init())
    {
        while (true)
        {
        }
    }

    StorageConfig cfg{};
    bool hasStoredConfig = storage.load(cfg);

    uint32_t startupBaudRate = 9600;
    if (hasStoredConfig && cfg.uartBaudRate != 0)
    {
        startupBaudRate = cfg.uartBaudRate;
    }

    static UartLayer uart;
    uart.init(startupBaudRate);

    static DeviceManager devManager(uart.getBaudRatePtr());

    if (hasStoredConfig)
    {
        importConfig(cfg, devManager);
    }

    static Application app(devManager, parser.getErrorCount(), storage);
    static SpinelProtocolManager manager(rxQueue, uart, app);
    static TemperatureTaskContext tempCtx{&devManager, &rtos_i2c_handle};

    if (xTaskCreate(TemperatureTask, "TempTask", 256, &tempCtx, 1, nullptr) != pdPASS)
    {
        while (true)
            ;
    }

    rxTimeoutTimer = xTimerCreate("RxTimeout", pdMS_TO_TICKS(Timeouts::INTER_BYTE_MS), pdFALSE,
                                  nullptr, RxTimeoutCallback);

    if (xTaskCreate(SpinelManagerTask, "SpinelTask", 1024, &manager, configMAX_PRIORITIES - 2,
                    nullptr) != pdPASS)
    {
        while (true)
            ;
    }

    vTaskStartScheduler();

    while (true)
        ;
    return 0;
}
