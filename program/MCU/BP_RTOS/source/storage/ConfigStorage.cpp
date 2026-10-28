#include "ConfigStorage.h"
#include <cstring>

alignas(512) static uint8_t s_flashBuffer[512];

bool ConfigStorage::isErased(const void *data, uint32_t size)
{
    const uint8_t *p = static_cast<const uint8_t *>(data);

    for (uint32_t i = 0; i < size; i++)
    {
        if (p[i] != 0xFF)
        {
            return false;
        }
    }

    return true;
}

bool ConfigStorage::init()
{
    if (initialized)
    {
        return true;
    }

    std::memset(&flashConfig, 0, sizeof(flashConfig));

    if (FLASH_Init(&flashConfig) != kStatus_Success)
    {
        return false;
    }

    initialized = true;
    return true;
}

bool ConfigStorage::isRecordValid(const StorageConfig &cfg) const
{
    if (cfg.magic != MAGIC)
    {
        return false;
    }

    if (cfg.sequence == 0xFFFFFFFF)
    {
        return false;
    }

    if (cfg.uartBaudRate == 0xFFFFFFFF)
    {
        return false;
    }

    return true;
}

bool ConfigStorage::readRecord(uint32_t addr, StorageConfig &cfg) const
{
    std::memcpy(&cfg, reinterpret_cast<const void *>(addr), sizeof(StorageConfig));
    return isRecordValid(cfg);
}

uint32_t ConfigStorage::findLatestRecordAddress() const
{
    uint32_t bestAddr = 0;
    uint32_t bestSeq = 0;
    bool found = false;

    const uint32_t sectors[2] = {SECTOR0_ADDR, SECTOR1_ADDR};

    // The newest valid record is selected by the highest sequence number.
    for (uint32_t sector : sectors)
    {
        for (uint32_t i = 0; i < RECORDS_PER_SECTOR; i++)
        {
            const uint32_t addr = sector + i * RECORD_SIZE;
            StorageConfig cfg{};

            if (!readRecord(addr, cfg))
            {
                continue;
            }

            if (!found || cfg.sequence > bestSeq)
            {
                found = true;
                bestSeq = cfg.sequence;
                bestAddr = addr;
            }
        }
    }

    return bestAddr;
}

uint32_t ConfigStorage::findNextFreeSlot(uint32_t sectorAddr) const
{
    // Flash memory is erased to 0xFF, so an erased record means a free slot.
    for (uint32_t i = 0; i < RECORDS_PER_SECTOR; i++)
    {
        const uint32_t addr = sectorAddr + i * RECORD_SIZE;

        if (isErased(reinterpret_cast<const void *>(addr), RECORD_SIZE))
        {
            return addr;
        }
    }

    return 0;
}

bool ConfigStorage::eraseSector(uint32_t sectorAddr)
{
    if (!initialized)
    {
        return false;
    }

    status_t status = FLASH_Erase(&flashConfig,
                                  sectorAddr,
                                  SECTOR_SIZE,
                                  kFLASH_ApiEraseKey);

    return status == kStatus_Success;
}

bool ConfigStorage::writeRecord(uint32_t addr, const StorageConfig &cfg)
{
    if (!initialized)
    {
        return false;
    }

    // Flash is programmed in full RECORD_SIZE blocks, unused bytes stay erased.
    std::memset(s_flashBuffer, 0xFF, sizeof(s_flashBuffer));
    std::memcpy(s_flashBuffer, &cfg, sizeof(StorageConfig));

    status_t status = FLASH_Program(&flashConfig, addr, s_flashBuffer, sizeof(s_flashBuffer));

    if (status != kStatus_Success)
    {
        return false;
    }

    StorageConfig verify{};
    std::memcpy(&verify, reinterpret_cast<const void *>(addr), sizeof(StorageConfig));

    // Verify programmed data by reading it back from flash.
    return std::memcmp(&verify, &cfg, sizeof(StorageConfig)) == 0;
}

bool ConfigStorage::load(StorageConfig &cfg)
{
    if (!initialized)
    {
        return false;
    }

    const uint32_t addr = findLatestRecordAddress();
    if (addr == 0)
    {
        return false;
    }

    return readRecord(addr, cfg);
}

bool ConfigStorage::save(const StorageConfig &cfg)
{
    if (!initialized)
    {
        return false;
    }

    StorageConfig newCfg = cfg;
    newCfg.magic = MAGIC;

    uint32_t latestAddr = findLatestRecordAddress();
    uint32_t nextSequence = 0;
    uint32_t activeSector = SECTOR0_ADDR;

    if (latestAddr != 0)
    {
        StorageConfig latestCfg{};
        if (!readRecord(latestAddr, latestCfg))
        {
            return false;
        }

        nextSequence = latestCfg.sequence + 1;
        activeSector = (latestAddr >= SECTOR1_ADDR) ? SECTOR1_ADDR : SECTOR0_ADDR;
    }

    newCfg.sequence = nextSequence;

    uint32_t freeAddr = findNextFreeSlot(activeSector);
    if (freeAddr != 0)
    {
        return writeRecord(freeAddr, newCfg);
    }

    // If the active sector is full, erase the other sector and continue there.
    const uint32_t otherSector = (activeSector == SECTOR0_ADDR) ? SECTOR1_ADDR : SECTOR0_ADDR;

    if (!eraseSector(otherSector))
    {
        return false;
    }

    return writeRecord(otherSector, newCfg);
}
