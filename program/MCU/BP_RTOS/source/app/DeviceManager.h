#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

struct Device
{
    const uint32_t *baudRate = nullptr;
    const char *deviceInfo = nullptr;
    int16_t lastTemp32 = 0;
    uint16_t lastTempRaw = 0;
    bool isConnected = false;
    std::array<uint8_t, 16> userData;
    std::array<uint8_t, 8> prodData;

    uint8_t spinelAddr = 0xFF;
    uint8_t i2cAddr = 0xFF;
    uint8_t statusByte = 0;
    bool configEnabled = false;
    bool checksumEnabled = true;
    bool needReset = false;
};

class DeviceManager
{
private:
    static constexpr size_t DEVICE_COUNT = 8;
    std::array<Device, DEVICE_COUNT> devs;

public:
    DeviceManager(const uint32_t *baudRatePtr);
    Device *getDevice(uint8_t spinelAddr);
    Device *getFirstActiveDevice();
    uint32_t getBaudRate() const;
    std::array<Device, DEVICE_COUNT> &getDevices();
    const std::array<Device, DEVICE_COUNT> &getDevices() const;
};