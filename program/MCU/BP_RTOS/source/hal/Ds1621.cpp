#include "Ds1621.h"

static constexpr uint8_t DS1621_CMD_READ_TEMP = 0xAA;
static constexpr uint8_t DS1621_CMD_ACCESS_CONFIG = 0xAC;
static constexpr uint8_t DS1621_CMD_START_CONVERT = 0xEE;

// Continuous conversion mode, other configuration bits remain cleared.
static constexpr uint8_t DS1621_CONFIG_CONTINUOUS = 0x00;

static TempStatus mapI2cStatus(status_t status)
{
    if (status == kStatus_Success)
    {
        return TempStatus::OK;
    }
    else if (status == kStatus_LPI2C_Nak)
    {
        return TempStatus::NAK;
    }
    else if (status == kStatus_LPI2C_Timeout)
    {
        return TempStatus::TIMEOUT;
    }
    else
    {
        return TempStatus::ERROR;
    }
}

static TempStatus ds1621_WriteCommand(lpi2c_rtos_handle_t *handle,
                                      uint8_t addr,
                                      uint8_t cmd)
{
    lpi2c_master_transfer_t xfer{};
    xfer.slaveAddress = addr;
    xfer.direction = kLPI2C_Write;
    xfer.subaddress = cmd;
    xfer.subaddressSize = 1;
    xfer.data = nullptr;
    xfer.dataSize = 0;
    xfer.flags = kLPI2C_TransferDefaultFlag;

    return mapI2cStatus(LPI2C_RTOS_Transfer(handle, &xfer));
}

bool ds1621_Init(lpi2c_rtos_handle_t *handle, uint8_t addr)
{
    uint8_t cfg = DS1621_CONFIG_CONTINUOUS;

    // Write DS1621 configuration register.
    lpi2c_master_transfer_t cfgXfer{};
    cfgXfer.slaveAddress = addr;
    cfgXfer.direction = kLPI2C_Write;
    cfgXfer.subaddress = DS1621_CMD_ACCESS_CONFIG;
    cfgXfer.subaddressSize = 1;
    cfgXfer.data = &cfg;
    cfgXfer.dataSize = 1;
    cfgXfer.flags = kLPI2C_TransferDefaultFlag;

    status_t status = LPI2C_RTOS_Transfer(handle, &cfgXfer);
    if (status != kStatus_Success)
    {
        return false;
    }

    // Start temperature conversion after configuration.
    TempStatus startStatus = ds1621_WriteCommand(handle, addr, DS1621_CMD_START_CONVERT);
    return startStatus == TempStatus::OK;
}

TempStatus ds1621_ReadTemperature(lpi2c_rtos_handle_t *handle,
                                  uint8_t i2cAddr,
                                  uint16_t &rawOut,
                                  int16_t &temp32Out)
{
    uint8_t rx[2] = {0};

    lpi2c_master_transfer_t xfer{};
    xfer.slaveAddress = i2cAddr;
    xfer.direction = kLPI2C_Read;
    xfer.subaddress = DS1621_CMD_READ_TEMP;
    xfer.subaddressSize = 1;
    xfer.data = rx;
    xfer.dataSize = 2;
    xfer.flags = kLPI2C_TransferDefaultFlag;

    status_t status = LPI2C_RTOS_Transfer(handle, &xfer);
    if (status != kStatus_Success)
    {
        return mapI2cStatus(status);
    }

    rawOut = (static_cast<uint16_t>(rx[0]) << 8) | rx[1];

    // DS1621 returns a signed 9-bit value with 0.5 °C resolution.
    int16_t temp2 = static_cast<int16_t>(
        (static_cast<uint16_t>(rx[0]) << 1) |
        ((rx[1] & 0x80) ? 1 : 0));

    if (temp2 & 0x0100)
    {
        temp2 |= 0xFE00;
    }

    // Internal protocol format stores temperature as value * 32.
    temp32Out = static_cast<int16_t>(temp2 * 16);

    return TempStatus::OK;
}
