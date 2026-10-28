#include "DeviceManager.h"

DeviceManager::DeviceManager(const uint32_t *baudRatePtr)
{
    std::array<uint8_t, 16> defaultUser = {'K', 'O', 'T', 'E', 'L', 'N', 'A', ' ', 'X', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    std::array<uint8_t, 8> defaultProd = {'B', 'A', 'K', ' ', '2', '0', '2', '6'};

    // Each virtual Spinel address is mapped to one physical I2C sensor address.
    uint8_t spinelAddrs[DEVICE_COUNT] = {'0', '1', '2', '3', '4', '5', '6', '7'};
    uint8_t i2cAddrs[DEVICE_COUNT] = {0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F};

    static const char *const sensorInfos[DEVICE_COUNT] = {
        "TMP275_0; v1.00; f97 66",
        "TMP275_1; v1.00; f97 66",
        "TMP275_2; v1.00; f97 66",
        "TMP275_3; v1.00; f97 66",
        "TMP275_4; v1.00; f97 66",
        "TMP275_5; v1.00; f97 66",
        "TMP275_6; v1.00; f97 66",
        "TMP275_7; v1.00; f97 66"};

    for (size_t i = 0; i < DEVICE_COUNT; i++)
    {
        // All devices reference the same current UART baud rate.
        devs[i].baudRate = baudRatePtr;
        devs[i].deviceInfo = sensorInfos[i];
        devs[i].lastTemp32 = 0;
        devs[i].lastTempRaw = 0;
        devs[i].isConnected = true;
        devs[i].userData = defaultUser;
        devs[i].prodData = defaultProd;
        devs[i].spinelAddr = spinelAddrs[i];
        devs[i].i2cAddr = i2cAddrs[i];
        devs[i].statusByte = 0x00;
        devs[i].configEnabled = false;
    }
}

Device *DeviceManager::getDevice(uint8_t spinelAddress)
{
    for (auto &dev : devs)
    {
        if (dev.spinelAddr == spinelAddress)
        {
            return &dev;
        }
    }
    return nullptr;
}

Device *DeviceManager::getFirstActiveDevice()
{
    for (auto &dev : devs)
    {
        if (dev.isConnected)
        {
            return &dev;
        }
    }
    return nullptr;
}

uint32_t DeviceManager::getBaudRate() const
{
    if (devs.empty() || devs[0].baudRate == nullptr)
    {
        return 0;
    }
    return *devs[0].baudRate;
}

std::array<Device, DeviceManager::DEVICE_COUNT> &DeviceManager::getDevices()
{
    return devs;
}

const std::array<Device, DeviceManager::DEVICE_COUNT> &DeviceManager::getDevices() const
{
    return devs;
}