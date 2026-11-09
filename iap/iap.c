#include "iap.h"
#include "main.h"
#include "cmsis_armcc.h"
#include "drv_flash.h"
#include "md5.h"

#define COPY_FLASH_BUFFER_SIZE  1024
#define COPY_FLASH_COUNT        (APP_TOTAL_SIZE / (sizeof(uint32_t) * COPY_FLASH_BUFFER_SIZE))

typedef void (*_func)(void);

__weak void app_wdg_feed(void);

void System_Reinitial(void)
{
  HAL_DeInit();
  /* disable all peripherals clock. */
  RCC->AHB1ENR = (1 << 20); /* 20: F4 CCMDAT ARAMEN. */
  RCC->AHB2ENR = 0;
  RCC->AHB3ENR = 0;
  RCC->APB1ENR = 0;
  RCC->APB2ENR = 0;

  /* Switch to default cpu clock. */
  RCC->CFGR = 0; 
  /* Disable MPU */
  MPU->CTRL &= ~MPU_CTRL_ENABLE_Msk;
  /* disable and clean up all interrupts. */
  for(int i = 0; i < 8; i++)
  {
    /* disable interrupts. */
    NVIC->ICER[i] = 0xFFFFFFFF;
    /* clean up interrupts flags. */
    NVIC->ICPR[i] = 0xFFFFFFFF;
  }
}
int IAP_AddressCheck(uint32_t app_address)
{
  if(((*(__IO uint32_t *)app_address) & 0x2FFE0000 ) == 0x20000000)
  {
    DBG_IAP("check 0x%x ok!", app_address);
    return 0;
  }
  else
  {
    DBG_IAP("check 0x%x error!!!!!!!", app_address);
    return -1;
  }
  
}

/*从备份区复制固件到运行区*/
int IAP_CopyAppFromBackupApp(void)
{
  uint32_t *pSrc, *pDst;
  uint32_t count;
  uint32_t backup_stack = *(uint32_t *)BACKUP_APP_BASE_ADDRESS;
  uint32_t backup_entry = *(uint32_t *)(BACKUP_APP_BASE_ADDRESS + 4);
  
  DBG_IAP("Copy App from Backup to App Area\r\n");
  DBG_IAP("Backup stack: 0x%x, entry: 0x%x\r\n", backup_stack, backup_entry);
  
  // 验证备份区固件
  if(IAP_AddressCheck(BACKUP_APP_BASE_ADDRESS) != 0)
  {
    DBG_IAP("Backup App check error!\r\n");
    return -1;
  }
  
  // 擦除运行区
  DBG_IAP("Erasing app area...\r\n");
  stm32_flash_erase(APP_BASE_ADDRESS, APP_TOTAL_SIZE);
  
  // 复制固件 (按字复制，提高效率)
  pSrc = (uint32_t *)BACKUP_APP_BASE_ADDRESS;
  pDst = (uint32_t *)APP_BASE_ADDRESS;
  count = APP_TOTAL_SIZE / sizeof(uint32_t);
  
  DBG_IAP("Copying %u words...\r\n", count);
  
  for(uint32_t i = 0; i < count; i++)
  {
    stm32_flash_write((uint32_t)pDst, (uint8_t *)pSrc, sizeof(uint32_t));
    pSrc++;
    pDst++;
    if((i % COPY_FLASH_BUFFER_SIZE) == 0)
    {
      //app_wdg_feed();  // 喂狗，防止超时
    }
  }
  
  DBG_IAP("Copy App Over\r\n");
  
  // 检查复制后的运行区
  uint32_t app_stack = *(uint32_t *)APP_BASE_ADDRESS;
  uint32_t app_entry = *(uint32_t *)(APP_BASE_ADDRESS + 4);
  DBG_IAP("App stack: 0x%x, entry: 0x%x\r\n", app_stack, app_entry);
  
  // 验证复制结果
  if(IAP_AddressCheck(APP_BASE_ADDRESS) != 0)
  {
    DBG_IAP("App copy check error!\r\n");
    // 尝试直接跳转，不进行地址检查
    DBG_IAP("Forcing jump to backup app...\r\n");
    return -2;
  }
  
  return 0;
}

// ==================== 升级标志操作 ====================

// 读取升级标志（从 0x080FF000）
int upgrade_flag_read(upgrade_flag_t *flag)
{
    if (flag == RT_NULL) return -1;

    uint8_t *pSrc = (uint8_t *)UPGRADE_FLAG_ADDRESS;
    uint8_t *pDst = (uint8_t *)flag;
    for (uint32_t i = 0; i < sizeof(upgrade_flag_t); i++)
        pDst[i] = pSrc[i];

    DBG_IAP("Read flag: status=%d, size=%u, crc=0x%08X",
            flag->upgrade_status, flag->firmware_size, flag->firmware_crc);
    return 0;
}

int upgrade_flag_write(upgrade_flag_t *flag)
{
    if (flag == NULL) return -1;

    DBG_IAP("Write flag: status=%d, size=%u, crc=0x%08X",
            flag->upgrade_status, flag->firmware_size, flag->firmware_crc);

    app_wdg_feed();

    // 1. 备份配置区前 16KB 到静态缓冲区（实际配置数据远小于 32KB）
    uint32_t *src = (uint32_t *)CONFIG_PARAM_ADDRESS;
    for (uint32_t i = 0; i < GATEWAY_CONFIG_SIZE / 4; i++)
    {
        uint32_t val = src[i];
        gateway_config_backup[i * 4]     = (uint8_t)(val);
        gateway_config_backup[i * 4 + 1] = (uint8_t)(val >> 8);
        gateway_config_backup[i * 4 + 2] = (uint8_t)(val >> 16);
        gateway_config_backup[i * 4 + 3] = (uint8_t)(val >> 24);
    }

    app_wdg_feed();

    // 2. 擦除 Sector 11
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    FLASH_EraseInitTypeDef eraseInit;
    eraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;
    eraseInit.Sector       = FLASH_SECTOR_11;
    eraseInit.NbSectors    = 1;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    uint32_t sectorError = 0;
    if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) != HAL_OK)
    {
        DBG_IAP("Erase sector 11 failed!");
        HAL_FLASH_Lock();
        return -1;
    }

    app_wdg_feed();  // 擦除后立即喂狗

    // 3. 恢复配置区前 GATEWAY_CONFIG_SIZE 字节(16KB)
    for (uint32_t i = 0; i < GATEWAY_CONFIG_SIZE / 4; i++)
    {
        uint32_t word = gateway_config_backup[i * 4] |
                       ((uint32_t)gateway_config_backup[i * 4 + 1] << 8) |
                       ((uint32_t)gateway_config_backup[i * 4 + 2] << 16) |
                       ((uint32_t)gateway_config_backup[i * 4 + 3] << 24);
        if (word != 0xFFFFFFFF)
        {
            stm32_flash_write(CONFIG_PARAM_ADDRESS + i * 4, (uint8_t *)&word, sizeof(uint32_t));
        }
    }

    // 4. 写入升级标志
    stm32_flash_write(UPGRADE_FLAG_ADDRESS, (uint8_t *)flag, sizeof(upgrade_flag_t));

    // 5. 验证gateway_config是否被破坏
    //BASE_CONFIG_READ;
    rt_kprintf("Verify gateway config info: \r\n");
    //rt_kprintf("Gateway ID: %s\r\n", Config_GetGatewayID());
    //rt_kprintf("Project ID: %s\r\n", Config_GetProjectID());
    //rt_kprintf("Freq Group: %d\r\n", Config_GetLoraparam()->loraFreqGroup);

    HAL_FLASH_Lock();
    DBG_IAP("Flag written successfully.\r\n");
    return 0;
}
// 清除升级标志（设为 IDLE 状态）
int upgrade_flag_clear(void)
{
    upgrade_flag_t flag;
    memset(&flag, 0xFF, sizeof(flag));
    flag.upgrade_status = UPGRADE_STATUS_IDLE;
    return upgrade_flag_write(&flag);
}

// ==================== CRC 校验 ====================

// 计算固件 CRC32
uint32_t IAP_CalcFirmwareCRC(uint32_t addr, uint32_t size)
{
    uint32_t crc = 0xFFFFFFFF;
    uint8_t *pData = (uint8_t *)addr;

    for (uint32_t i = 0; i < size; i++)
    {
        crc ^= pData[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        if ((i % 4096) == 0)
            app_wdg_feed();
    }
    return ~crc;
}

int IAP_CalcFirmwareMD5(uint32_t addr, uint32_t size, char md5_hex[33])
{
    md5_context_t ctx;
    rt_uint8_t md5_result[16];
    rt_uint8_t *pData = (rt_uint8_t *)addr;
    rt_uint32_t offset = 0;

    if (md5_hex == RT_NULL || size == 0)
        return -1;

    md5_starts(&ctx);

    while (offset < size)
    {
        rt_uint32_t chunk = (size - offset) > 4096 ? 4096 : (size - offset);
        md5_update(&ctx, pData + offset, chunk);
        offset += chunk;
        app_wdg_feed();
    }

    md5_finish(&ctx, md5_result);

    for (int i = 0; i < 16; i++)
    {
        rt_snprintf(md5_hex + i * 2, 3, "%02x", md5_result[i]);
    }
    md5_hex[32] = '\0';

    return 0;
}

__weak void app_wdg_feed(void)
{
}