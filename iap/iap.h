#ifndef __IAP_H
#define __IAP_H

#include <rtthread.h>

//FLASH 扇区的起始地址
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) 	//扇区0起始地址, 16 Kbytes
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) 	//扇区1起始地址, 16 Kbytes
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) 	//扇区2起始地址, 16 Kbytes
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) 	//扇区3起始地址, 16 Kbytes
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) 	//扇区4起始地址, 64 Kbytes
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) 	//扇区5起始地址, 128 Kbytes

#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) 	//扇区6起始地址, 128 Kbytes
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) 	//扇区7起始地址, 128 Kbytes

#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) 	//扇区8起始地址, 128 Kbytes

#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) 	//扇区9起始地址, 128 Kbytes
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) 	//扇区10起始地址,128 Kbytes

#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) 	//扇区11起始地址,128 Kbytes

#define DBG_IAP(...)      {\
                            rt_kprintf("[IAP]:");\
                            rt_kprintf(__VA_ARGS__);\
                            rt_kprintf("\r\n");\
                          }
/***********************************
1. Flash 分区布局（STM32F407 1MB Flash）
分区	             起始地址	      大小	   说明
Bootloader	      0x08000000     64KB    启动引导程序          （section0-3）
App存储区(分区A）  0x08010000     256KB	  应用存储区 A          （section4-6）
App下载区(分区B)   0x08060000	    256KB   应用下载区 B          （section7-8）
Lora下载区         0x080A0000     256KB   Lora固件下载区        （section9-10）
网关配置区         0x080E0000     124KB   存储网关配置等参数     （section11）
升级标志存储       0x080FF000     4KB     存储升级标志upgrade_flag       
2. 升级过程
   |- 下载固件到下载区（分区B）
   |- 校验下载区固件完整性
   |- 更新升级标志（0x080FF000）为：UPGRADE_STATUS_DOWNLOADED
   |- 重启应用
3. Bootloader启动后，检查升级标志：
   |- 如果标志为：UPGRADE_STATUS_DOWNLOADED，则执行升级流程：
      |- 校验下载区固件完整性
      |- 擦除应用存储区（分区A）
      |- 将下载区固件复制到应用存储区（分区A）
      |- 校验应用存储区固件完整性
      |- 更新升级标志为：UPGRADE_STATUS_READY
      |- 跳转到应用存储区（分区A）运行
   |- 如果标志为：UPGRADE_STATUS_IDLE，则直接跳转到应用存储区（分区A）运行
4. 应用启动后，检查升级标志：
   |- 如果标志为：UPGRADE_STATUS_READY
      |- 说明升级成功，上报升级状态：真正完成
      |- 清除升级标志（0x080FF000）清除升级标志为：UPGRADE_STATUS_IDLE
   |- 否则正常启动
***********************************/

#define APP_TOTAL_SIZE             0x40000	//256KB
#define IAP_APP_PKG_SIZE           2048

#define BOOTLOADER_ADDRESS         ADDR_FLASH_SECTOR_0
// Bootloader占用 0x08000000-0x0800FFFF (Sector 0-3, 64KB)
// 应用运行地址: 0x08010000 (Sector 4-6, 256KB)
#define APP_BASE_ADDRESS           ADDR_FLASH_SECTOR_4

// 升级包备份存储地址: 0x08060000 (Sector 7-8, 256KB)
#define BACKUP_APP_BASE_ADDRESS    ADDR_FLASH_SECTOR_7

// LoRa节点固件存储地址: 0x080A0000 (Sector 9-10, 256KB)
#define LORA_APP_BASE_ADDRESS      ADDR_FLASH_SECTOR_9
                          
// 配置参数存储地址: 0x080E0000 (Sector 11)
#define CONFIG_PARAM_ADDRESS       ADDR_FLASH_SECTOR_11
                         
// 升级标志存储地址: 0x080FF000 (Sector 11 末尾 4KB)
#define UPGRADE_FLAG_ADDRESS       ((uint32_t)0x080FF000)

#define GATEWAY_CONFIG_SIZE       (16 * 1024)

// 升级状态枚举
typedef enum {
    UPGRADE_STATUS_IDLE       = 0x00,    // 空闲（无升级）
    UPGRADE_STATUS_DOWNLOADED = 0x01,    // 固件下载已完成，等待升级
    UPGRADE_STATUS_READY      = 0x02,    // bootloader已升级完成，准备重启应用
    UPGRADE_STATUS_FAILED     = 0x03     // 升级失败
} UpgradeStatus;

// 升级参数结构体（参数区存储）
#pragma pack(1)
typedef struct {
    uint8_t upgrade_status;       // 升级状态（UpgradeStatus）
    uint8_t app_type;             // 应用类型: 0 - bootloader,   1 - gateway
    uint8_t upgrade_type;         // 升级类型: 0 - 全量升级,      1 - 差分升级
    uint8_t upgrade_magic_number; // 升级魔数
    uint32_t firmware_size;       // 固件大小
    uint32_t firmware_crc;        // 固件CRC校验值
    uint8_t  reserved[16];        // 预留空间
} upgrade_flag_t;

#pragma pack()

// 网关配置区备份缓冲区，用于写入升级标志时保护网关配置区数据
static uint8_t gateway_config_backup[GATEWAY_CONFIG_SIZE];

int IAP_AddressCheck(uint32_t app_address);
int IAP_CopyAppFromBackupApp(void);
int upgrade_flag_read(upgrade_flag_t *flag);
int upgrade_flag_write(upgrade_flag_t *flag);
int upgrade_flag_clear(void);
uint32_t IAP_CalcFirmwareCRC(uint32_t addr, uint32_t size);
int IAP_CalcFirmwareMD5(uint32_t addr, uint32_t size, char md5_hex[33]);
#endif