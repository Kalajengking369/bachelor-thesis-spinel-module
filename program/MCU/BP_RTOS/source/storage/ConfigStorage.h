#pragma once

#include "StorageConfig.h"
#include <cstdint>
#include "fsl_flash.h"

class ConfigStorage
{
private:
    static constexpr uint32_t MAGIC = 0x43464731; // "CFG1"
    static constexpr uint32_t SECTOR_SIZE = 8192;
    static constexpr uint32_t RECORD_SIZE = 512;
    static constexpr uint32_t RECORDS_PER_SECTOR = SECTOR_SIZE / RECORD_SIZE;

    // Two flash sectors are used as rotating storage for configuration records.
    static constexpr uint32_t SECTOR0_ADDR = 0x001FC000;
    static constexpr uint32_t SECTOR1_ADDR = 0x001FE000;

    static_assert(sizeof(StorageConfig) <= RECORD_SIZE,
                  "StorageConfig must fit into one flash record");

    flash_config_t flashConfig{};
    bool initialized = false;

    static bool isErased(const void *data, uint32_t size);

    bool readRecord(uint32_t addr, StorageConfig &cfg) const;
    bool writeRecord(uint32_t addr, const StorageConfig &cfg);
    bool eraseSector(uint32_t sectorAddr);

    bool isRecordValid(const StorageConfig &cfg) const;
    uint32_t findLatestRecordAddress() const;
    uint32_t findNextFreeSlot(uint32_t sectorAddr) const;

public:
    bool init();
    bool load(StorageConfig &cfg);
    bool save(const StorageConfig &cfg);
};
