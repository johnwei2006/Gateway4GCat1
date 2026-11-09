#ifndef FLASH_BLOCK_H
#define FLASH_BLOCK_H

#include <stdint.h>
#include <stdbool.h>
#include "abstract_flash_external.h"

/************************ 块核心配置 ************************/
#define FLASH_SIZE_BYTES        16384   // 模拟Flash总大小（16KB）: FM25V01/16Kbyte
#define BLOCK_SIZE_MGR_AREA     2560    // 块管理区大小（2.5KB，存储位图+索引）
#define BLOCK_SIZE_BYTES        96      // 块大小：96Byte/0x60
#define BLOCK_NUM_TOTAL         144     // 总块数（96*144=13824Byte=13.5KB）
#define BLOCK_START_ADDRESS_MANAGER_AREA    0x00    // 块管理区起始地址
#define BLOCK_START_ADDRESS_CONFIG_AREA     0xA00   // 块配置数据区起始地址:2560/2.5K

// 块类型枚举
typedef enum {
    BLOCK_TYPE_NONE           = 0,    // 未使用
    BLOCK_TYPE_GATEWAY_CONFIG = 1,    // 网关信息（包括网关Lora）
    BLOCK_TYPE_NODE_CONFIG    = 2,    // 节点信息（包括节点Lora）
    BLOCK_TYPE_NODE_SENSOR    = 3,    // 节点传感器
    BLOCK_TYPE_NODE_DEVICE    = 4,    // 节点设备
    BLOCK_TYPE_TASK           = 5     // 任务配置
} block_type_t;

// 块索引项（映射块号与业务属性）
typedef struct {
    uint16_t        block_num;              // 块编号（0~BLOCK_TOTAL_NUM-1）
    block_type_t    type;                   // 块类型
    uint8_t         id[11];                 // 块ID（匹配GATEWAY_ID_LENGTH、NODE_ID_LENGTH）
    uint8_t         channel;                // 通道号（如节点传感器的通道号），网关信息/节点信息块该字段无效，置-1
} block_index_item_t;   // 16 bytes

// 块管理头部（存储块状态与索引）
typedef struct {
    uint8_t         bitmap[BLOCK_NUM_TOTAL / 8]; // 块占用位图（1=占用，0=空闲） 144/8=18 bytes
    uint16_t        used_block_num;            // 已使用块数
    block_index_item_t block_index[BLOCK_NUM_TOTAL]; // 块索引表, 16Byte，16*144=2464Byte   
    uint32_t        crc32;                     // 管理区CRC32（不含自身）
} block_manager_t;  // 18+2+144*16+4=2328 bytes

static flash_status flash_erase(uint32_t addr, uint32_t len);
static flash_status flash_write(uint32_t addr, const uint8_t *data, uint32_t len);
static flash_status flash_read(uint32_t addr, uint8_t *data, uint32_t len);
static flash_status block_manager_save(void);
static flash_status block_manager_load(void);
static uint32_t crc32_calc(const uint8_t *data, uint32_t len);

/************************ 核心接口 ************************/
// 初始化块管理模块（加载/初始化管理区）
int flash_block_manager_init(void);

flash_status flash_block_reset(void);

// 分配块（指定类型、节点ID、序号）
// 返回值：成功=块号，失败=0xFFFF
uint16_t block_alloc(block_type_t type, const uint8_t *node_id, uint8_t channel, int16_t specific_block_index);

// 释放块（按块号）
flash_status block_free(uint16_t block_num);

// 写入块数据（按块号，自动校验）
flash_status block_write(uint16_t block_num, const void *data, uint8_t data_size);

// 读取块数据（按块号，自动校验）
flash_status block_read(uint16_t block_num, void *data, uint8_t data_size);

// 根据属性查找块号
// 返回值：成功=块号，失败=0xFFFF
uint16_t block_find(block_type_t type, const uint8_t *node_id, uint8_t channel);

// 返回值：成功=块号，失败=0xFFFF
uint16_t block_find_by_type(block_type_t type, uint16_t occurrence);

// 返回值：成功=删除块数，失败=0xFFFF
uint16_t block_delete_by_type_and_id(block_type_t type, const uint8_t *id);

// 返回值：成功=块数，失败=0xFFFF
uint16_t block_size_by_type(block_type_t type);
// 获取块使用状态
void block_print_status(uint16_t *used, uint16_t *total, bool print_detail);

flash_status read_block_item_by_block_num(uint16_t block_num, block_index_item_t *item);

#endif // FLASH_BLOCK_H



































