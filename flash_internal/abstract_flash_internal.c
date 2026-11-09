/*
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe:
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
*/

#include <string.h>
#include "abstract_flash_internal.h"
#include "stm32f4xx_flash.h"
#include "stm32l1xx_flash.h"

/**
 * 读FLASH
 * @param  address 地址
 * @param  buffer  存放读取的数据
 * @param  size    要读取的数据大小，单位字节
 */
void flash_internal_read(uint32_t address, uint8_t *buffer, uint32_t size)
{
#ifdef USING_STM32F4XX_FLASH
    stm32f4xx_flash_read(address, buffer, size);
#endif
#ifdef USING_STM32L1XX_FLASH
    stm32l1xx_flash_read(address, buffer, size);
#endif
}
/**
 * 写FLASH
 * @param  address    写入起始地址
 * @param  buffer     待写入的数据
 * @param  NumToWrite 要写入的数据量
 */
void flash_internal_write(uint32_t address, uint8_t *buffer, uint32_t size)
{
#ifdef USING_STM32F4XX_FLASH
    stm32f4xx_flash_write(address, buffer, size);
#endif
#ifdef USING_STM32L1XX_FLASH
    stm32l1xx_flash_write(address, buffer, size);
#endif
}
