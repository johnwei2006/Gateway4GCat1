#ifndef __STM32F4XX_FLASH_H
#define __STM32F4XX_FLASH_H

#include "stm32f4xx.h"
#include <stdio.h>

#define BOARD_NUM_ADDR 0x0800C000
/* FLASH大小：512K */
#define STM32F4_FLASH_SIZE  0X00080000UL
/* STM32 FLASH的起始地址 */
#define STM32F4_FLASH_BASE 0x08000000
/* FLASH结束地址 */
#define STM32F4_FLASH_END   (STM32F4_FLASH_BASE | STM32F4_FLASH_SIZE)

#define STM32F4_FLASH_WAITETIME 50000       //FLASH等待超时时间


//FLASH 扇区的起始地址
#define ADDR_FLASH_SECTOR_0 ((uint32_t)0x08000000)  //扇区0起始地址, 16 Kbytes
#define ADDR_FLASH_SECTOR_1 ((uint32_t)0x08004000)  //扇区1起始地址, 16 Kbytes
#define ADDR_FLASH_SECTOR_2 ((uint32_t)0x08008000)  //扇区2起始地址, 16 Kbytes
#define ADDR_FLASH_SECTOR_3 ((uint32_t)0x0800C000)  //扇区3起始地址, 16 Kbytes
#define ADDR_FLASH_SECTOR_4 ((uint32_t)0x08010000)  //扇区4起始地址, 64 Kbytes
#define ADDR_FLASH_SECTOR_5 ((uint32_t)0x08020000)  //扇区5起始地址, 128 Kbytes
#define ADDR_FLASH_SECTOR_6 ((uint32_t)0x08040000)  //扇区6起始地址, 128 Kbytes
#define ADDR_FLASH_SECTOR_7 ((uint32_t)0x08060000)  //扇区7起始地址, 128 Kbytes
#define ADDR_FLASH_SECTOR_8 ((uint32_t)0x08080000)  //扇区8起始地址, 128 Kbytes
#define ADDR_FLASH_SECTOR_9 ((uint32_t)0x080A0000)  //扇区9起始地址, 128 Kbytes
#define ADDR_FLASH_SECTOR_10 ((uint32_t)0x080C0000) //扇区10起始地址,128 Kbytes
#define ADDR_FLASH_SECTOR_11 ((uint32_t)0x080E0000) //扇区11起始地址,128 Kbytes

uint32_t stm32f4xx_flash_read_word(uint32_t addr);
void stm32f4xx_flash_read(uint32_t address, uint8_t *buffer, uint32_t size);
uint8_t stm32f4xx_flash_write(uint32_t address, uint8_t *buffer, uint32_t size);
static uint8_t stm32f4xx_get_flash_sector(uint32_t addr);
#endif
