#pragma once
#include <array>
#include <cstdint>

struct StorageDeviceConfig
{
	uint8_t spinelAddr;
	uint8_t checksumEnabled;
	uint8_t reserved[2];
	std::array<uint8_t, 16> userData;
};

struct StorageConfig
{
	uint32_t magic;
	uint32_t sequence;
	uint32_t uartBaudRate;
	std::array<StorageDeviceConfig, 8> devs;
};
