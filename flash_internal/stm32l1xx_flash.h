#ifndef __STM32L1XX_FLASH_H
#define __STM32L1XX_FLASH_H

//#include "stm32l1xx.h"
#include <stdio.h>
#include <stdint.h>

#define BOARD_NUM_ADDR 0x0800C000
/* FLASH大小：512K */
#define STM32L1_FLASH_SIZE  0X00080000UL
/* STM32 FLASH的起始地址 */
#define STM32L1_FLASH_BASE 0x08000000
/* FLASH结束地址 */
#define STM32L1_FLASH_END   (STM32L1_FLASH_BASE | STM32L1_FLASH_SIZE)

/* FLASH页大小：2K */
#define STM32L1_FLASH_PAGE_SIZE (2 * 1024U)

#define STM32L1_FLASH_WAITETIME 50000       //FLASH等待超时时间

/* FLASH总页数 */
#define STM32L1_FLASH_PAGE_NUM  (STM32L1_FLASH_SIZE / STM32L1_FLASH_PAGE_SIZE)


void stm32l1xx_flash_read(uint32_t address, uint8_t *buffer, uint32_t size);
void stm32l1xx_flash_write(uint32_t address, uint8_t *buffer, uint32_t size);
void stm32l1xx_flash_write_nocheck(uint32_t address, uint8_t *buffer, uint32_t size);

#endif
