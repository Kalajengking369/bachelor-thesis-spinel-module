#pragma once
#include <cstdint>
#include "StorageConfig.h"

class DeviceManager;

void exportConfig(const DeviceManager &devs, StorageConfig &out);
void importConfig(const StorageConfig &in, DeviceManager &devs);
