#pragma once
#include <cstdint>

namespace SpinelAddrBin
{
    constexpr uint8_t UNIVERSAL = 0xFE;
    constexpr uint8_t BROADCAST = 0xFF;
}

namespace SpinelAddrAscii
{
    constexpr char UNIVERSAL = '$';
    constexpr char BROADCAST = '%';
}

namespace Protocol
{
    constexpr uint8_t BIN97 = 0x61;
    constexpr uint8_t ASCII66 = 'B';
    constexpr uint8_t NONE = 0x00;
    constexpr uint8_t CR = 0x0D;
    constexpr uint8_t PRE = 0x2A;
}

namespace InstructionBin
{
    constexpr uint8_t NONE = 0x00;
    constexpr uint8_t GET_TEMP = 0x51;
    constexpr uint8_t GET_RAW = 0x5F;
    constexpr uint8_t RESET = 0xE3;
    constexpr uint8_t SET_STATUS = 0xE1;
    constexpr uint8_t WRITE_USER_DATA = 0xE2;
    constexpr uint8_t SET_CONFIG = 0xE4;
    constexpr uint8_t SET_COMMS = 0xE0;
    constexpr uint8_t SET_CHECKSUM = 0xEE;
    constexpr uint8_t READ_COMMS = 0xF0;
    constexpr uint8_t READ_STATUS = 0xF1;
    constexpr uint8_t READ_USER_DATA = 0xF2;
    constexpr uint8_t READ_INFO = 0xF3;
    constexpr uint8_t READ_ERRORS = 0xF4;
    constexpr uint8_t READ_PROD_INFO = 0xFA;
    constexpr uint8_t READ_CHECKSUM = 0xFE;
}

namespace InstructionAscii
{
    constexpr const char *NONE = nullptr;
    constexpr const char *GET_TEMP = "TR";
    constexpr const char *RESET = "RE";
    constexpr const char *SET_CONFIG = "E";
    constexpr const char *SET_ADDR = "AS";
    constexpr const char *SET_SPEED = "SS";
    constexpr const char *SET_STATUS = "SW";
    constexpr const char *READ_STATUS = "SR";
    constexpr const char *WRITE_USER_DATA = "DW";
    constexpr const char *READ_USER_DATA = "DR";
    constexpr const char *READ_INFO = "?";
}

namespace AckBin
{
    constexpr uint8_t OK = 0x00;
    constexpr uint8_t ERROR = 0x01;
    constexpr uint8_t INVALID_INSTRUCTION = 0x02;
    constexpr uint8_t INVALID_DATA = 0x03;
    constexpr uint8_t UNAUTHORIZED_WRITE = 0x04;
    constexpr uint8_t DEVICE_FAILURE = 0x05;
    constexpr uint8_t AUTO_DATATRANSFER = 0x0C;
    constexpr uint8_t AUTO_LOG_VALUE_CHANGE = 0x0D;
    constexpr uint8_t AUTO_MEASUREMENT = 0x0E;
    constexpr uint8_t AUTO_OTHER = 0x0F;
}

namespace AckAscii
{
    constexpr char OK = '0';
    constexpr char ERROR = '1';
    constexpr char INVALID_INSTRUCTION = '2';
    constexpr char INVALID_DATA = '3';
    constexpr char UNAUTHORIZED_WRITE = '4';
    constexpr char DEVICE_FAILURE = '5';
    constexpr char AUTO_DATATRANSFER = 'C';
    constexpr char AUTO_LOG_VALUE_CHANGE = 'D';
    constexpr char AUTO_MEASUREMENT = 'E';
    constexpr char AUTO_OTHER = 'F';
}

namespace Timeouts
{
    constexpr int INTER_BYTE_MS = 100;
    constexpr int RESPONSE_MS = 1000;
}