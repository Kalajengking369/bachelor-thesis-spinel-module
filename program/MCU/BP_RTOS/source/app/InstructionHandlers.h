#pragma once
#include <array>
#include "SpinelBuffers.h"

struct Device;

class DeviceManager;

class InstructionHandlers
{
private:
    volatile uint8_t &errorCount;
    DeviceManager &devs;

    using HandlerFn = TxBuffer (InstructionHandlers::*)(SpinelBufferView payload, bool isAscii, Device &dev);

    struct HandlerEntry
    {
        uint8_t instBin;
        const char *instAscii;
        HandlerFn handler;
        bool protectedCmd;
    };

    static const std::array<HandlerEntry, 16> handlerTable;
    TxBuffer handleGetTemp(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleGetRaw(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleReset(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleSetStatus(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleEnableConfig(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleSetComms(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleSetSpeed(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleReadComms(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleReadStatus(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleReadInfo(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleWriteUserData(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleReadUserData(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleReadErrors(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleReadProdInfo(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleReadChecksum(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleSetChecksum(SpinelBufferView payload, bool isAscii, Device &dev);
    TxBuffer handleUnknown(SpinelBufferView payload, bool isAscii);

public:
    InstructionHandlers(DeviceManager &deviceManager, volatile uint8_t &errCount);
    TxBuffer process(SpinelBufferView payload, bool isAscii, Device &dev, bool isUnicast);
};