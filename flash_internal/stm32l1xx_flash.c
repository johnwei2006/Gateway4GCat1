/*
#include <flash_internal/stm32l1xx_flash.h>
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe:
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
*/

#include <string.h>
#include <stdint.h>
#include "stm32l1xx_flash.h"

/**
 * 读FLASH
 * @param  address 地址
 * @param  buffer  存放读取的数据
 * @param  size    要读取的数据大小，单位字节
 */
void stm32l1xx_flash_read(uint32_t address, uint8_t *buffer, uint32_t size)
{
    uint32_t i = 0;
    if (!buffer || address < STM32L1_FLASH_BASE || address >= STM32L1_FLASH_END)
        return ;

    while (i < size && (address + i <= (STM32L1_FLASH_END - 1)))
    {
        *buffer ++  = *(volatile uint8_t *)(address + i++);
    }
}
/**
 * 写FLASH
 * @param  address    写入起始地址
 * @param  buffer     待写入的数据
 * @param  NumToWrite 要写入的数据量
 */
void stm32l1xx_flash_write(uint32_t address, uint8_t *buffer, uint32_t size)
{
    uint32_t wNumber = 0;         // 记录剩余要写入的数据量
    /* 非法地址 */
    if (address < STM32L1_FLASH_BASE || address > (STM32L1_FLASH_END - 2) || size == 0 || buffer == NULL || address % STM32L1_FLASH_PAGE_SIZE != 0)
        return ;
    /* 解锁FLASH */
    FLASH_Unlock();//解锁
    //FLASH_DataCacheCmd(DISABLE);//FLASH擦除期间，必须禁止数据缓存
    while(wNumber < size){
      volatile uint16_t value = *(uint16_t *)(buffer + wNumber);
      volatile uint32_t addr = address + wNumber;
      if(wNumber % STM32L1_FLASH_PAGE_SIZE == 0)  //
      {
        //fmc_page_erase(addr);
          FLASH_EraseSector(addr);
      }
      //fmc_halfword_program(addr, value);
      FLASH_ProgramWord(addr, value);
      wNumber += 2;
    }
    /* 加锁FLASH */
    //FLASH_DataCacheCmd(ENABLE);//FLASH擦除结束，开启数据缓存
    FLASH_Lock();//上锁
}

//WriteAddr:起始地址
//pBuffer:数据指针
//NumToWrite:半字(16位)数
void stm32l1xx_flash_write_nocheck(uint32_t address, uint8_t *buffer, uint32_t size)
{
    uint32_t i;
    for(i=0; i < size; i++)
    {
        FLASH_ProgramHalfWord(address, buffer[i]);
        address+=2;//地址增加2.
    }
}

