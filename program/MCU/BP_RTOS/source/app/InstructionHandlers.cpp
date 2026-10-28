#include "InstructionHandlers.h"

#include "FreeRTOS.h"
#include "ProtocolConstants.h"
#include "SpinelUtils.h"
#include "fsl_debug_console.h"
#include "task.h"

#include "DeviceManager.h"
#include <cstdio>
#include <cstring>

const std::array<InstructionHandlers::HandlerEntry, 16> InstructionHandlers::handlerTable = {{
    {InstructionBin::GET_TEMP, InstructionAscii::GET_TEMP, &InstructionHandlers::handleGetTemp, false},
    {InstructionBin::GET_RAW, InstructionAscii::NONE, &InstructionHandlers::handleGetRaw, false},
    {InstructionBin::RESET, InstructionAscii::RESET, &InstructionHandlers::handleReset, false},
    {InstructionBin::SET_STATUS, InstructionAscii::SET_STATUS, &InstructionHandlers::handleSetStatus, false},
    {InstructionBin::SET_CONFIG, InstructionAscii::SET_CONFIG, &InstructionHandlers::handleEnableConfig, false},
    {InstructionBin::SET_COMMS, InstructionAscii::SET_ADDR, &InstructionHandlers::handleSetComms, true},
    {InstructionBin::NONE, InstructionAscii::SET_SPEED, &InstructionHandlers::handleSetSpeed, true},
    {InstructionBin::READ_COMMS, InstructionAscii::NONE, &InstructionHandlers::handleReadComms, false},
    {InstructionBin::READ_ERRORS, InstructionAscii::NONE, &InstructionHandlers::handleReadErrors, false},
    {InstructionBin::READ_STATUS, InstructionAscii::READ_STATUS, &InstructionHandlers::handleReadStatus, false},
    {InstructionBin::WRITE_USER_DATA, InstructionAscii::WRITE_USER_DATA, &InstructionHandlers::handleWriteUserData, false},
    {InstructionBin::READ_USER_DATA, InstructionAscii::READ_USER_DATA, &InstructionHandlers::handleReadUserData, false},
    {InstructionBin::READ_INFO, InstructionAscii::READ_INFO, &InstructionHandlers::handleReadInfo, false},
    {InstructionBin::READ_CHECKSUM, InstructionAscii::NONE, &InstructionHandlers::handleReadChecksum, false},
    {InstructionBin::SET_CHECKSUM, InstructionAscii::NONE, &InstructionHandlers::handleSetChecksum, false},
    {InstructionBin::READ_PROD_INFO, InstructionAscii::NONE, &InstructionHandlers::handleReadProdInfo, false},
}};

InstructionHandlers::InstructionHandlers(DeviceManager &deviceManager, volatile uint8_t &errCount)
    : errorCount(errCount), devs(deviceManager) {}

TxBuffer InstructionHandlers::process(SpinelBufferView payload, bool isAscii, Device &dev,
                                      bool isUnicast)
{
    if (payload.isEmpty())
        return handleUnknown(payload, isAscii);
    HandlerFn handlerToCall = nullptr;
    bool isProtected = false;
    size_t instLen = 0;

    if (dev.isConnected == false)
    {
        TxBuffer tx{};
        tx.len = 0;
        tx.ackCode = (isAscii) ? AckAscii::DEVICE_FAILURE : AckBin::DEVICE_FAILURE;
        return tx;
    }

    for (const auto &entry : handlerTable)
    {
        if (isAscii)
        {
            if (entry.instAscii == InstructionAscii::NONE)
            {
                continue;
            }
            size_t cmdLen = strlen(entry.instAscii);

            if (payload.len >= cmdLen)
            {
                if (strncmp((const char *)payload.data, entry.instAscii, cmdLen) == 0)
                {
                    handlerToCall = entry.handler;
                    isProtected = entry.protectedCmd;
                    instLen = cmdLen;
                    break;
                }
            }
        }
        else
        {
            if (entry.instBin == InstructionBin::NONE)
            {
                continue;
            }

            if (payload[0] == entry.instBin)
            {
                handlerToCall = entry.handler;
                isProtected = entry.protectedCmd;
                instLen = 1;
                break;
            }
        }
    }

    if (!handlerToCall)
    {
        return handleUnknown(payload, isAscii);
    }

    TxBuffer tx;
    tx.len = 0;
    // Protected commands require unicast access and enabled configuration mode.
    bool needsUnicast = isProtected || (handlerToCall == &InstructionHandlers::handleEnableConfig);

    if (needsUnicast && !isUnicast)
    {
        tx.ackCode = isAscii ? AckAscii::UNAUTHORIZED_WRITE : AckBin::UNAUTHORIZED_WRITE;
        dev.configEnabled = false;
        return tx;
    }

    if (isProtected && !dev.configEnabled)
    {
        tx.ackCode = isAscii ? AckAscii::UNAUTHORIZED_WRITE : AckBin::UNAUTHORIZED_WRITE;
        dev.configEnabled = false;
        return tx;
    }

    // Remove instruction bytes, handler receives only command parameters.
    SpinelBufferView params = {payload.data + instLen, payload.len - instLen};

    tx = (this->*handlerToCall)(params, isAscii, dev);
    if (handlerToCall != &InstructionHandlers::handleEnableConfig)
    {
        dev.configEnabled = false;
    }
    return tx;
}

TxBuffer InstructionHandlers::handleReset(SpinelBufferView payload, bool isAscii, Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 0)
    {
        tx.ackCode = isAscii ? AckAscii::INVALID_DATA : AckBin::INVALID_DATA;
        return tx;
    }

    dev.needReset = true;

    tx.ackCode = isAscii ? AckAscii::OK : AckBin::OK;
    tx.triggerReset = true;
    return tx;
}

TxBuffer InstructionHandlers::handleSetStatus(SpinelBufferView payload, bool isAscii, Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 1)
    {
        tx.ackCode = isAscii ? AckAscii::INVALID_DATA : AckBin::INVALID_DATA;
        return tx;
    }

    tx.ackCode = isAscii ? AckAscii::OK : AckBin::OK;

    uint8_t newStatus = payload[0];

    dev.statusByte = newStatus;

    return tx;
}

TxBuffer InstructionHandlers::handleEnableConfig(SpinelBufferView payload, bool isAscii,
                                                 Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 0)
    {
        tx.ackCode = isAscii ? AckAscii::INVALID_DATA : AckBin::INVALID_DATA;
        return tx;
    }
    dev.configEnabled = true;
    tx.ackCode = isAscii ? AckAscii::OK : AckBin::OK;
    return tx;
}

TxBuffer InstructionHandlers::handleGetTemp(SpinelBufferView payload, bool isAscii, Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 0)
    {
        tx.ackCode = isAscii ? AckAscii::INVALID_DATA : AckBin::INVALID_DATA;
        return tx;
    }

    int16_t currentTemp = dev.lastTemp32;

    if (isAscii)
    {
        tx.ackCode = AckAscii::OK;

        // Convert internal temp * 32 format to tenths of a degree.
        int16_t temp10;
        if (currentTemp >= 0)
        {
            temp10 = static_cast<int16_t>((currentTemp * 10 + 16) / 32);
        }
        else
        {
            temp10 = static_cast<int16_t>((currentTemp * 10 - 16) / 32);
        }

        bool neg = (temp10 < 0);
        uint16_t absTemp10 = neg ? static_cast<uint16_t>(-temp10) : static_cast<uint16_t>(temp10);

        uint16_t whole = absTemp10 / 10;
        uint8_t frac = absTemp10 % 10;

        tx.data[0] = neg ? '-' : '+';
        tx.data[1] = static_cast<uint8_t>('0' + ((whole / 100) % 10));
        tx.data[2] = static_cast<uint8_t>('0' + ((whole / 10) % 10));
        tx.data[3] = static_cast<uint8_t>('0' + (whole % 10));
        tx.data[4] = '.';
        tx.data[5] = static_cast<uint8_t>('0' + frac);
        tx.data[6] = 'C';
        tx.len = 7;
    }
    else
    {
        tx.ackCode = AckBin::OK;

        tx.data[0] = (static_cast<uint16_t>(currentTemp) >> 8) & 0xFF;
        tx.data[1] = static_cast<uint16_t>(currentTemp) & 0xFF;
        tx.len = 2;
    }

    return tx;
}

TxBuffer InstructionHandlers::handleGetRaw(SpinelBufferView payload, bool, Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 0)
    {
        tx.ackCode = AckBin::INVALID_DATA;
        return tx;
    }

    tx.ackCode = AckBin::OK;
    tx.data[0] = (dev.lastTempRaw >> 8) & 0xFF;
    tx.data[1] = dev.lastTempRaw & 0xFF;
    tx.len = 2;

    return tx;
}

TxBuffer InstructionHandlers::handleSetComms(SpinelBufferView payload, bool isAscii, Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    tx.ackCode = isAscii ? AckAscii::OK : AckBin::OK;
    if (isAscii)
    {
        if (payload.len != 1)
        {
            tx.ackCode = AckAscii::INVALID_DATA;
            return tx;
        }

        dev.spinelAddr = payload[0];
        return tx;
    }

    if (payload.len != 2)
    {
        tx.ackCode = AckBin::INVALID_DATA;
        return tx;
    }

    uint8_t newAddr = payload[0];
    uint8_t speedCode = payload[1];

    uint32_t newBaudRate = SpinelUtils::getUartSpeed(speedCode);
    if (newBaudRate == 0)
    {
        tx.ackCode = AckBin::INVALID_DATA;
        return tx;
    }

    dev.spinelAddr = newAddr;
    tx.newBaud = newBaudRate;
    tx.triggerFlashSave = true;
    return tx;
}

TxBuffer InstructionHandlers::handleSetSpeed(SpinelBufferView payload, bool, Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 1)
    {
        tx.ackCode = AckBin::INVALID_DATA;
        return tx;
    }

    tx.ackCode = AckAscii::OK;

    uint8_t speedCodeChar = payload[0];
    uint8_t speedCodeBin = SpinelUtils::convertAsciiToBin(speedCodeChar);
    uint32_t newBaudRate = SpinelUtils::getUartSpeed(speedCodeBin);

    if (newBaudRate == 0)
    {
        tx.ackCode = AckAscii::INVALID_DATA;
        return tx;
    }

    tx.newBaud = newBaudRate;
    tx.triggerFlashSave = true;
    return tx;
}

TxBuffer InstructionHandlers::handleReadComms(SpinelBufferView payload, bool, Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 0)
    {
        tx.ackCode = AckBin::INVALID_DATA;
        return tx;
    }

    tx.len = 2;
    tx.ackCode = AckBin::OK;
    tx.data[0] = dev.spinelAddr;
    tx.data[1] = SpinelUtils::getSpinelSpeedCode(*dev.baudRate);
    return tx;
}

TxBuffer InstructionHandlers::handleReadStatus(SpinelBufferView payload, bool isAscii,
                                               Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 0)
    {
        tx.ackCode = isAscii ? AckAscii::INVALID_DATA : AckBin::INVALID_DATA;
        return tx;
    }

    uint8_t status = dev.statusByte;
    tx.ackCode = isAscii ? AckAscii::OK : AckBin::OK;
    tx.data[0] = status;
    tx.len = 1;

    return tx;
}

TxBuffer InstructionHandlers::handleReadInfo(SpinelBufferView payload, bool isAscii, Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 0)
    {
        tx.ackCode = isAscii ? AckAscii::INVALID_DATA : AckBin::INVALID_DATA;
        return tx;
    }

    struct DevState
    {
        bool connected;
        uint8_t addr;
    };
    std::array<DevState, 8> states;

    // Copy device states while interrupts/tasks cannot modify them.
    taskENTER_CRITICAL();
    auto &devices = devs.getDevices();
    for (size_t i = 0; i < devices.size(); i++)
    {
        states[i].connected = devices[i].isConnected;
        states[i].addr = devices[i].spinelAddr;
    }
    taskEXIT_CRITICAL();

    tx.ackCode = isAscii ? AckAscii::OK : AckBin::OK;
    size_t offset = 0;
    if (isAscii)
    {
        tx.data[offset++] = ' ';
    }
    // ASCII discovery format: "slot/address slot/address".
    for (size_t i = 0; i < states.size(); i++)
    {
        if (states[i].connected)
        {
            if (isAscii)
            {
                if (offset + 4 > tx.data.size())
                    break;
                tx.data[offset++] = '0' + (uint8_t)i;
                tx.data[offset++] = '/';
                tx.data[offset++] = states[i].addr;
                tx.data[offset++] = ' ';
            }
            else
            {
                if (offset + 4 > tx.data.size())
                    break;
                tx.data[offset++] = '0' + static_cast<uint8_t>(i);
                tx.data[offset++] = '/';
                tx.data[offset++] = states[i].addr;
                tx.data[offset++] = ' ';
            }
        }
    }

    if (offset > (isAscii ? 1 : 0))
    {
        offset--; // Remove trailing space
    }
    tx.len = offset;

    return tx;
}

TxBuffer InstructionHandlers::handleWriteUserData(SpinelBufferView payload, bool isAscii,
                                                  Device &dev)
{
    TxBuffer tx;
    tx.len = 0;
    tx.ackCode = isAscii ? AckAscii::OK : AckBin::OK;

    if (payload.len < 2)
    {
        tx.ackCode = isAscii ? AckAscii::INVALID_DATA : AckBin::INVALID_DATA;
        return tx;
    }

    uint8_t pos = payload[0];
    if (isAscii)
    {
        pos = SpinelUtils::convertAsciiToBin(pos);
    }

    if (pos >= dev.userData.size())
    {
        tx.ackCode = isAscii ? AckAscii::INVALID_DATA : AckBin::INVALID_DATA;
        return tx;
    }

    size_t dataLen = payload.len - 1;
    size_t copyLen = std::min(dataLen, dev.userData.size() - pos);

    for (size_t i = 0; i < copyLen; i++)
    {
        dev.userData[pos + i] = payload[1 + i];
    }

    tx.triggerFlashSave = true;
    return tx;
}

TxBuffer InstructionHandlers::handleReadUserData(SpinelBufferView payload, bool isAscii,
                                                 Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 0)
    {
        tx.ackCode = isAscii ? AckAscii::INVALID_DATA : AckBin::INVALID_DATA;
        return tx;
    }

    size_t len = dev.userData.size();

    if (isAscii)
    {
        tx.ackCode = AckAscii::OK;
        while (len > 1 && dev.userData[len - 1] == ' ')
            len--;
    }
    else
    {
        tx.ackCode = AckBin::OK;
    }
    std::memcpy(tx.data.data(), dev.userData.data(), len);
    tx.len = len;

    return tx;
}

TxBuffer InstructionHandlers::handleReadErrors(SpinelBufferView payload, bool, Device &)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 0)
    {
        tx.ackCode = AckBin::INVALID_DATA;
        return tx;
    }

    tx.ackCode = AckBin::OK;
    tx.len = 1;
    // Reading the error counter also clears it.
    taskENTER_CRITICAL();
    tx.data[0] = errorCount;
    errorCount = 0;
    taskEXIT_CRITICAL();
    return tx;
}

TxBuffer InstructionHandlers::handleReadProdInfo(SpinelBufferView payload, bool, Device &dev)
{
    TxBuffer tx;
    tx.len = 0;

    if (payload.len != 0)
    {
        tx.ackCode = AckBin::INVALID_DATA;
        return tx;
    }

    size_t len = dev.prodData.size();
    tx.ackCode = AckBin::OK;
    std::memcpy(tx.data.data(), dev.prodData.data(), len);
    tx.len = len;

    return tx;
}

TxBuffer InstructionHandlers::handleUnknown(SpinelBufferView payload, bool isAscii)
{
    TxBuffer tx;
    tx.len = 0;

    tx.ackCode = isAscii ? AckAscii::INVALID_INSTRUCTION : AckBin::INVALID_INSTRUCTION;
    return tx;
}

TxBuffer InstructionHandlers::handleReadChecksum(SpinelBufferView payload, bool, Device &dev)
{
    TxBuffer tx;
    tx.len = 1;
    tx.ackCode = AckBin::OK;
    if (payload.len != 0)
    {
        tx.ackCode = AckBin::INVALID_DATA;
        return tx;
    }

    tx.data[0] = (dev.checksumEnabled) ? 1 : 0;
    return tx;
}

TxBuffer InstructionHandlers::handleSetChecksum(SpinelBufferView payload, bool, Device &dev)
{
    TxBuffer tx;
    tx.len = 0;
    tx.ackCode = AckBin::OK;
    if (payload.len != 1)
    {
        tx.ackCode = AckBin::INVALID_DATA;
        return tx;
    }
    dev.checksumEnabled = (payload.data[0]) ? true : false;
    tx.triggerFlashSave = true;
    return tx;
}
