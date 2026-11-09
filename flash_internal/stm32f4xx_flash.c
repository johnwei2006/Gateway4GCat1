/*
#include <flash_internal/stm32f4xx_flash.h>
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe:
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
*/

#include <string.h>
#include "stm32f4xx_flash.h"

uint8_t stm32f4xx_flash_read_byte(uint32_t addr)
{
    return *(__IO uint8_t *)addr;
}

uint16_t stm32f4xx_flash_read_halfword(uint32_t addr)
{
    return *(__IO uint16_t *)addr;
}

uint32_t stm32f4xx_flash_read_word(uint32_t addr)
{
    return *(__IO uint32_t *)addr;
}
/**
 * 读FLASH
 * @param  address 地址
 * @param  buffer  存放读取的数据
 * @param  size    要读取的数据大小，单位字节
 */
void stm32f4xx_flash_read(uint32_t address, uint8_t *buffer, uint32_t size)
{
    if (!buffer || address < STM32F4_FLASH_BASE || address >= STM32F4_FLASH_END)
        return ;

    for(uint32_t i = 0; i < size; i++)
    {
        *buffer++ = stm32f4xx_flash_read_byte(address);
        address++;
    }
}
/**
 * 写FLASH
 * @param  address    写入起始地址
 * @param  buffer     待写入的数据
 * @param  size       要写入的数据量
 */
uint8_t stm32f4xx_flash_write(uint32_t address, uint8_t *buffer, uint32_t size)
{
    /* address validation */
    if (address < STM32F4_FLASH_BASE || address >= STM32F4_FLASH_END || size == 0 || buffer == NULL)
        return -1;
    uint32_t first_sector   = 0;
    uint32_t sector_size    = 0;

    uint32_t sector_error   = 0;

    static FLASH_EraseInitTypeDef EraseInitStruct;

    /* FLASH unlock ********************************/
    HAL_FLASH_Unlock();

    first_sector    = stm32f4xx_get_flash_sector(address);
    sector_size     = stm32f4xx_get_flash_sector(address + size)- first_sector + 1;

    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector        = first_sector;
    EraseInitStruct.NbSectors     = sector_size;
    /* Flash sector erase */
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &sector_error) != HAL_OK) {
       /*Erase error process here */
       return -1;
    }

    uint32_t write_in_address       = address;
    __IO uint8_t write_in_data8     = 0;
    uint16_t pointer                = 0;

    while (write_in_address < address + size) {
        write_in_data8 =  *(uint8_t *)(buffer + pointer);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, write_in_address, write_in_data8) == HAL_OK) {
            write_in_address ++;
            pointer          ++;
        } else {
            return -1;
        }
    }

    /* Flash lock */
    HAL_FLASH_Lock();

    /* Flash data check***************************************/
    /*  memory_check_status = 0:
       memory_check_status != 0: 写入的数据错误，其值为错误的个数 */
    write_in_address                    = address;
    pointer                             = 0;
    __IO uint32_t memory_check_status   = 0;
    uint8_t data8_in_buffer             = 0;

    while (write_in_address < address + size) {
        write_in_data8     = *(__IO uint8_t*)write_in_address;
        data8_in_buffer    = *(uint8_t *)(buffer + pointer);


       if (write_in_data8 != data8_in_buffer) {
           memory_check_status++;
       }

       write_in_address ++;
       pointer ++;
    }
    if (memory_check_status) {
       return -1;
    } else {
       return 0;
    }
}

static uint8_t stm32f4xx_get_flash_sector(uint32_t addr)
{
    if (addr < ADDR_FLASH_SECTOR_1)
        return FLASH_SECTOR_0;
    else if (addr < ADDR_FLASH_SECTOR_2)
        return FLASH_SECTOR_1;
    else if (addr < ADDR_FLASH_SECTOR_3)
        return FLASH_SECTOR_2;
    else if (addr < ADDR_FLASH_SECTOR_4)
        return FLASH_SECTOR_3;
    else if (addr < ADDR_FLASH_SECTOR_5)
        return FLASH_SECTOR_4;
    else if (addr < ADDR_FLASH_SECTOR_6)
        return FLASH_SECTOR_5;
    else if (addr < ADDR_FLASH_SECTOR_7)
        return FLASH_SECTOR_6;
    else if (addr < ADDR_FLASH_SECTOR_8)
        return FLASH_SECTOR_7;
    else if (addr < ADDR_FLASH_SECTOR_9)
        return FLASH_SECTOR_8;
    else if (addr < ADDR_FLASH_SECTOR_10)
        return FLASH_SECTOR_9;
    else if (addr < ADDR_FLASH_SECTOR_11)
        return FLASH_SECTOR_10;
    return FLASH_SECTOR_11;
}
