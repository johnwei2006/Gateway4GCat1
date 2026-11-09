#ifndef __W25Q128_FLASH_H
#define __W25Q128_FLASH_H

#include <stdio.h>
#include <stdint.h>
#include "abstract_flash_external.h"

flash_status w25q128_flash_read(uint32_t address, uint8_t *buffer, uint32_t size);
flash_status w25q128_flash_write(uint32_t address, uint8_t *buffer, uint32_t size);
flash_status w25q128_init(void);
static uint16_t w25q128_read_device_id(void);
#endif
