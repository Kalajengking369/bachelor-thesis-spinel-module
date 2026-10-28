#pragma once

#include "fsl_lpi2c_freertos.h"
#include <stdint.h>

enum class TempStatus
{
    OK,
    NAK,
    TIMEOUT,
    ERROR
};

bool ds1621_Init(lpi2c_rtos_handle_t *handle, uint8_t addr);

TempStatus ds1621_ReadTemperature(lpi2c_rtos_handle_t *handle,
                                  uint8_t i2cAddr,
                                  uint16_t &rawOut,
                                  int16_t &temp32Out);