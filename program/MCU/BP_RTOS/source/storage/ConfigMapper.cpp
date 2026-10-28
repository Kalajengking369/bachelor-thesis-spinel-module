#include "ConfigMapper.h"
#include "app/DeviceManager.h"
#include <cstddef>

void exportConfig(const DeviceManager &devs, StorageConfig &out)
{
    out = {};
    out.uartBaudRate = devs.getBaudRate();

    const auto &devices = devs.getDevices();

    for (size_t i = 0; i < out.devs.size(); ++i)
    {
        out.devs[i].spinelAddr = devices[i].spinelAddr;
        out.devs[i].checksumEnabled = devices[i].checksumEnabled ? 1 : 0;
        out.devs[i].userData = devices[i].userData;
    }
}

void importConfig(const StorageConfig &in, DeviceManager &devs)
{
    auto &devices = devs.getDevices();

    for (size_t i = 0; i < in.devs.size(); ++i)
    {
        devices[i].spinelAddr = in.devs[i].spinelAddr;
        devices[i].checksumEnabled = (in.devs[i].checksumEnabled != 0);
        devices[i].userData = in.devs[i].userData;
    }
}
