#pragma once
#include <cstdint>

namespace SpinelUtils
{
    constexpr uint8_t convertAsciiToBin(uint8_t c)
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'Z')
            return c - 'A' + 10;
        if (c >= 'a' && c <= 'z')
            return c - 'a' + 10;
        if (c == '%')
            return 0xFF;
        if (c == '$')
            return 0xFE;
        return c;
    }

    constexpr char convertBinToAscii(uint8_t n)
    {
        if (n <= 9)
            return n + '0';
        if (n >= 10 && n <= 15)
            return n - 10 + 'A';
        return '?';
    }

    // Converts UART baud rate to Spinel speed code.
    constexpr uint8_t getSpinelSpeedCode(uint32_t baudRate)
    {
        switch (baudRate)
        {
        case 1200:
            return 0x03;
        case 2400:
            return 0x04;
        case 4800:
            return 0x05;
        case 9600:
            return 0x06;
        case 19200:
            return 0x07;
        case 38400:
            return 0x08;
        case 57600:
            return 0x09;
        case 115200:
            return 0x0A;
        default:
            return 0xFF;
        }
    }

    // Converts Spinel speed code back to UART baud rate.
    constexpr uint32_t getUartSpeed(uint8_t code)
    {
        switch (code)
        {
        case 0x03:
            return 1200;
        case 0x04:
            return 2400;
        case 0x05:
            return 4800;
        case 0x06:
            return 9600;
        case 0x07:
            return 19200;
        case 0x08:
            return 38400;
        case 0x09:
            return 57600;
        case 0x0A:
            return 115200;
        default:
            return 0;
        }
    }

    constexpr bool isAsciiFrame(uint8_t frm)
    {
        return frm < 97; // 0-96 ASCII, 97-255 binary
    }
}
