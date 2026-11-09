/*
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe:
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
*/

#include <string.h>
#include "abstract_flash_external.h"
#include "fm25_flash.h"
#include "w25q128_flash.h"

/**
 * read FLASH
 * @param  address :start address to read
 * @param  buffer  :data to be read
 * @param  size    :read data size(byte)
 */
flash_status flash_external_read(uint32_t address, uint8_t *buffer, uint32_t size)
{
#ifdef USING_FLASH_FM25
    return FRAM_Read(address, buffer, size);
#endif
#ifdef USING_FLASH_W25Q128
    return w25q128_flash_read(address, buffer, size);
#endif
}
/**
 * write FLASH
 * @param  address    :start address to write
 * @param  buffer     :data to be written
 * @param  size       :write data size(byte)
 */
flash_status flash_external_write(uint32_t address, uint8_t *buffer, uint32_t size)
{
#ifdef USING_FLASH_FM25
    return FRAM_Write(address, buffer, size);
#endif
#ifdef USING_FLASH_W25Q128
    return w25q128_flash_write(address, buffer, size);
#endif
}

int flash_init(void)
{
	#ifdef USING_FLASH_FM25
		FRAM_Init();
	#endif
    /* 解锁FLASH */
    // if (HAL_FLASH_Unlock() != HAL_OK)
    // {
    //     return FLASH_ERROR_LOCK;
    // }
    
    /* 清除所有挂起标志 */
    // __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
    //                        FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    
    return 0;    
}

INIT_COMPONENT_EXPORT(flash_init);