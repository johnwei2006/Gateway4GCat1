#ifndef __ABSTRACT_FLASH_EXTERNAL_H
#define __ABSTRACT_FLASH_EXTERNAL_H

#include <stdio.h>
#include <stdint.h>

/*
 * Flash storage structure:
 *    Data Type                 Start Address    End Address  Volume    Accumulated Volume
 * 1. Gateway Config          : 0x00000000U  --- 0x00001000U  4K        :4K
 * 2. Devices Config          : 0x00001001U  --- 0x00010000U  60K       :64K
 * 3. Node OTA Data Cache     ：0x00010001U  --- 0x00090000U  512K      :576K
 * 4. Node Downlink Data Cache：0x00090001U  --- 0x00900000U  448K      :1024K
 * 5. Node Uplink Data Cache  : 0x00900001U  --- 0x10000000U  N*512K    :(N/2+1)*1024K
 */
#define FLASH_GATEWAY_CONFIG 0x00000000U
#define FLASH_DEVICES_CONFIG 0x00000801U
#define FLASH_OTA_CACHE      0x00100000U
#define FLASH_DOWNLINK_CACHE 0x00104801U
#define FLASH_UPLINK_CACHE   0x00004801U


#ifndef USING_FLASH_FM25
#define USING_FLASH_FM25
#endif

#ifndef USING_FLASH_W25Q128
//#define USING_FLASH_W25Q128
#endif

/**
  * @brief  FLASH操作结果状态枚举
  */
typedef enum
{
    FLASH_OK = 0,
    FLASH_ERROR_NOT_FOUND,
    FLASH_ERROR_ACCUPIED,// FLASH已被占用
    FLASH_ERROR_FULL, // FLASH已用尽
    FLASH_ERROR_CRC,// CRC校验失败
    FLASH_ERROR_PARAM,  // 参数错误
    FLASH_ERROR_INVALID
} flash_status;

flash_status flash_external_read(uint32_t address, uint8_t *buffer, uint32_t size);
flash_status flash_external_write(uint32_t address, uint8_t *buffer, uint32_t size);
int flash_init(void);
#endif
