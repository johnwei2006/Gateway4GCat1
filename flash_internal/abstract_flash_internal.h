#ifndef __ABSTRACT_FLASH_INTERNAL_H
#define __ABSTRACT_FLASH_INTERNAL_H

#include <stdio.h>
#include <stdint.h>

#ifndef USING_STM32F4XX_FLASH
#define USING_STM32F4XX_FLASH
#endif

void flash_internal_read(uint32_t address, uint8_t *buffer, uint32_t size);
void flash_internal_write(uint32_t address, uint8_t *buffer, uint32_t size);

#endif
