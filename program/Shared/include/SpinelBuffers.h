#pragma once
#include "ProtocolConstants.h"

#include <array>
#include <cstddef>
#include <cstdint>

struct SpinelBufferView
{
    const uint8_t *data;
    size_t len;

    uint8_t operator[](size_t index) const
    {
        return data[index];
    }

    bool isEmpty() const
    {
        return len == 0;
    }
};

struct TxBuffer
{
    static constexpr size_t CAPACITY = 256;
    std::array<uint8_t, CAPACITY> data{};
    size_t len = 0;
    uint32_t newBaud = 0;
    uint8_t addr = 0xFF;
    uint8_t ackCode = 0x00;
    bool triggerFlashSave = false;
    bool triggerReset = false;
    bool hasResponse = false;
};

namespace RxFlags
{
    constexpr uint8_t OVERFLOW = 0x01;
    constexpr uint8_t CRC_ERROR = 0x02;
    constexpr uint8_t SHORT_FRAME = 0x04;
}

struct RxBuffer
{
    static constexpr size_t CAPACITY = 256;
    std::array<uint8_t, CAPACITY> data{};
    size_t len = 0;
    uint8_t flags = 0;
    void append(uint8_t byte)
    {
        if (len >= CAPACITY)
        {
            flags |= RxFlags::OVERFLOW;
            return;
        }
        data[len++] = byte;
    }

    void clear()
    {
        len = 0;
        flags = 0;
    }

    SpinelBufferView getView() const
    {
        return {data.data(), len};
    }

    uint8_t operator[](size_t index) const
    {
        return data[index];
    }
};

struct UnwrappedFrame
{
    SpinelBufferView payload;
    bool isAscii = false;
    bool frameValid = false;
};

struct UnwrappedSData
{
    SpinelBufferView payload;
    uint8_t addr;
    uint8_t sig = 0;
    bool addrValid = false;
    bool sigValid = false;
};