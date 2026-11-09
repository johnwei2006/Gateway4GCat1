#include "flash_block.h"
#include <string.h>
#include <rtthread.h>

/************************ 全局变量 ************************/
static block_manager_t  g_block_mgr;            // 块管理区缓存
static bool             g_mgr_inited = false;   // 管理区是否初始化

// 初始化块管理模块
int flash_block_manager_init(void)
{
    if (g_mgr_inited) return 0;

    // 尝试加载管理区
    flash_status ret = block_manager_load();
    if (ret == FLASH_OK) {
        g_mgr_inited = true;
        return 0;
    }

    // 加载失败，初始化管理区
    memset(&g_block_mgr, 0, sizeof(block_manager_t));
    g_block_mgr.used_block_num = 0;
    memset(g_block_mgr.bitmap, 0, sizeof(g_block_mgr.bitmap));
    for (uint16_t i = 0; i < BLOCK_NUM_TOTAL; i++) {
        g_block_mgr.block_index[i].block_num = i;
        g_block_mgr.block_index[i].type = BLOCK_TYPE_NONE;
        memset(g_block_mgr.block_index[i].id, 0, 11);
        g_block_mgr.block_index[i].channel = 0;
    }
    // 保存初始化后的管理区
    ret = block_manager_save();
    if (ret == FLASH_OK) {
        g_mgr_inited = true;
    }

    return ret == FLASH_OK ? 0 : -1;
}

// Flash擦除（按字节擦除，实际需按硬件最小擦除块对齐）
static flash_status flash_erase(uint32_t addr, uint32_t len)
{
    if (addr + len > FLASH_SIZE_BYTES) return FLASH_ERROR_PARAM;
    return flash_external_write(addr, NULL, len);
}

// Flash写入（仅能写0，需先擦除）
static flash_status flash_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if (addr + len > FLASH_SIZE_BYTES || data == NULL) return FLASH_ERROR_PARAM;
    return flash_external_write(addr, (uint8_t *)data, len);
}

// Flash读取
static flash_status flash_read(uint32_t addr, uint8_t *data, uint32_t len)
{
    if (addr + len > FLASH_SIZE_BYTES || data == NULL) return FLASH_ERROR_PARAM;
    return flash_external_read(addr, data, len);
}

/************************ CRC32工具函数 ************************/
#define CRC32_POLY 0xEDB88320
static uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0) return 0;
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? CRC32_POLY : 0);
        }
    }
    return ~crc;
}

/************************ 块管理核心逻辑 ************************/
// 保存管理区到Flash
static flash_status block_manager_save(void)
{
    // 更新管理区CRC
    g_block_mgr.crc32 = crc32_calc((uint8_t *)&g_block_mgr, sizeof(block_manager_t) - 4);

    // 擦除管理区（按1KB擦除）
    if (flash_erase(BLOCK_START_ADDRESS_MANAGER_AREA, BLOCK_SIZE_MGR_AREA) != FLASH_OK) {
        return FLASH_ERROR_INVALID;
    }

    // 写入管理区
    if (flash_write(BLOCK_START_ADDRESS_MANAGER_AREA, (uint8_t *)&g_block_mgr, sizeof(block_manager_t)) != FLASH_OK) {
        return FLASH_ERROR_INVALID;
    }

    return FLASH_OK;
}

// 加载管理区从Flash
static flash_status block_manager_load(void)
{
    // 读取管理区
    if (flash_read(BLOCK_START_ADDRESS_MANAGER_AREA, (uint8_t *)&g_block_mgr, sizeof(block_manager_t)) != FLASH_OK) {
        return FLASH_ERROR_INVALID;
    }

    // 校验CRC
    uint32_t crc = crc32_calc((uint8_t *)&g_block_mgr, sizeof(block_manager_t) - 4);
    if (crc != g_block_mgr.crc32) {
        return FLASH_ERROR_INVALID;
    }

    return FLASH_OK;
}

flash_status flash_block_reset(void){
    memset(&g_block_mgr, 0, sizeof(block_manager_t));
    g_block_mgr.used_block_num = 0;
    memset(g_block_mgr.bitmap, 0, sizeof(g_block_mgr.bitmap));
    for (uint16_t i = 0; i < BLOCK_NUM_TOTAL; i++) {
        g_block_mgr.block_index[i].block_num = i;
        g_block_mgr.block_index[i].type = BLOCK_TYPE_NONE;
        memset(g_block_mgr.block_index[i].id, 0, 11);
        g_block_mgr.block_index[i].channel = 0;
    }
    // 保存初始化后的管理区
    flash_status ret = block_manager_save();
    if (ret == FLASH_OK) {
        g_mgr_inited = true;
    }

    return ret;
}

// 分配块
uint16_t block_alloc(block_type_t type, const uint8_t *id, uint8_t channel, int16_t specific_block_index)
{
    if (!g_mgr_inited || type == BLOCK_TYPE_NONE || id == NULL) {
        return 0xFFFF;
    }

    if( specific_block_index != -1){
        // 分配指定块号
        uint16_t block_num = specific_block_index;
        uint8_t byte_idx = block_num / 8;
        uint8_t bit_idx = block_num % 8;
        if ((g_block_mgr.bitmap[byte_idx] & (1 << bit_idx)) == 0) {
            // 标记块为占用
            g_block_mgr.bitmap[byte_idx] |= (1 << bit_idx);
            g_block_mgr.used_block_num++;

            // 更新索引项
            g_block_mgr.block_index[block_num].type = type;
            memcpy(g_block_mgr.block_index[block_num].id, id, 11);
            g_block_mgr.block_index[block_num].channel = channel;

            // 保存管理区
            block_manager_save();
            return block_num;
        }else{
            return 0xFFFF; // 已被占用
        }
    }
    // 块已用尽
    if (g_block_mgr.used_block_num >= BLOCK_NUM_TOTAL) {
        return 0xFFFF;
    }

    // 遍历位图找空闲块
    for (uint16_t i = 0; i < BLOCK_NUM_TOTAL; i++) {
        uint8_t byte_idx = i / 8;
        uint8_t bit_idx = i % 8;
        if ((g_block_mgr.bitmap[byte_idx] & (1 << bit_idx)) == 0) {
            // 标记块为占用
            g_block_mgr.bitmap[byte_idx] |= (1 << bit_idx);
            g_block_mgr.used_block_num++;

            // 更新索引项
            g_block_mgr.block_index[i].type = type;
            memcpy(g_block_mgr.block_index[i].id, id, 11);
            g_block_mgr.block_index[i].channel = channel;

            // 保存管理区
            block_manager_save();
            return i;
        }
    }

    return 0xFFFF;
}

// 释放块
flash_status block_free(uint16_t block_num)
{
    if (!g_mgr_inited || block_num >= BLOCK_NUM_TOTAL) {
        return FLASH_ERROR_PARAM;
    }

    uint8_t byte_idx = block_num / 8;
    uint8_t bit_idx = block_num % 8;

    // 块未被占用
    if ((g_block_mgr.bitmap[byte_idx] & (1 << bit_idx)) == 0) {
        return FLASH_ERROR_NOT_FOUND;
    }

    // 标记块为空闲
    g_block_mgr.bitmap[byte_idx] &= ~(1 << bit_idx);
    g_block_mgr.used_block_num--;

    // 清空索引项
    g_block_mgr.block_index[block_num].type = BLOCK_TYPE_NONE;
    memset(g_block_mgr.block_index[block_num].id, 0, 11);
    g_block_mgr.block_index[block_num].channel = 0;

    // 保存管理区
    return block_manager_save();
}

// 写入块数据
flash_status block_write(uint16_t block_num, const void *data, uint8_t data_size)
{
    if (!g_mgr_inited || block_num >= BLOCK_NUM_TOTAL || data == NULL || data_size > BLOCK_SIZE_BYTES) {
        return FLASH_ERROR_PARAM;
    }

    // 计算块的Flash地址
    uint32_t flash_addr = BLOCK_START_ADDRESS_CONFIG_AREA + block_num * BLOCK_SIZE_BYTES;

    // 擦除块（按50Byte擦除，实际需对齐硬件擦除块）
    if (flash_erase(flash_addr, BLOCK_SIZE_BYTES) != FLASH_OK) {
        return FLASH_ERROR_INVALID;
    }

    // 写入块数据
    if (flash_write(flash_addr, data, data_size) != FLASH_OK) {
        return FLASH_ERROR_INVALID;
    }

    // 回读校验
    /*uint8_t buf[BLOCK_SIZE_BYTES] = {0};
    if (flash_read(flash_addr, buf, data_size) != FLASH_OK) {
        return FLASH_ERROR_INVALID;
    }
    if (memcmp(buf, data, data_size) != 0) {
        return FLASH_ERROR_INVALID;
    }*/

    return FLASH_OK;
}

// 读取块数据
flash_status block_read(uint16_t block_num, void *data, uint8_t data_size)
{
    if (!g_mgr_inited || block_num >= BLOCK_NUM_TOTAL || data == NULL || data_size > BLOCK_SIZE_BYTES) {
        return FLASH_ERROR_INVALID;
    }

    // 计算块的Flash地址
    uint32_t flash_addr = BLOCK_START_ADDRESS_CONFIG_AREA + block_num * BLOCK_SIZE_BYTES;

    // 读取块数据
    return flash_read(flash_addr, data, data_size);
}

// 查找块号
uint16_t block_find(block_type_t type, const uint8_t *id, uint8_t channel)
{
    if (!g_mgr_inited || type == BLOCK_TYPE_NONE || id == NULL) {
        return 0xFFFF;
    }

    // 遍历索引表查找
    for (uint16_t i = 0; i < BLOCK_NUM_TOTAL; i++) {
        if (g_block_mgr.block_index[i].type == type &&
            memcmp(g_block_mgr.block_index[i].id, id, 11) == 0 &&
            g_block_mgr.block_index[i].channel == channel) {
            return i;
        }
    }

    return 0xFFFF;
}

uint16_t block_find_by_type(block_type_t type, uint16_t occurrence){
    if (!g_mgr_inited || type == BLOCK_TYPE_NONE) {
        return 0xFFFF;
    }
    uint16_t count = 0;

    // 遍历索引表查找
    for (uint16_t i = 0; i < BLOCK_NUM_TOTAL; i++) {
        if (g_block_mgr.block_index[i].type == type) {
            count++;
            if (count == occurrence) {
                return i;
            }
        }
    }
    return 0xFFFF;
};

uint16_t block_delete_by_type_and_id(block_type_t type, const uint8_t *id){
    if (!g_mgr_inited || type == BLOCK_TYPE_NONE || id == NULL) {
        return NULL;
    }
    // 遍历索引表查找
    uint16_t found_block_size = 0;
    for (uint16_t i = 0; i < BLOCK_NUM_TOTAL; i++) {
        if (g_block_mgr.block_index[i].type == type &&
            memcmp(g_block_mgr.block_index[i].id, id, 11) == 0) {
            found_block_size++;
            block_free(i);
        }
    }
    return found_block_size;
};

uint16_t block_size_by_type(block_type_t type){
    if (!g_mgr_inited || type == BLOCK_TYPE_NONE) {
        return 0xFFFF;
    }
    uint16_t size = 0;
    // 遍历索引表查找
    for (uint16_t i = 0; i < BLOCK_NUM_TOTAL; i++) {
        if (g_block_mgr.block_index[i].type == type) {
            size++;
        }
    }
    return size;
};

// 获取块使用状态
void block_print_status(uint16_t *used, uint16_t *total, bool print_detail)
{
    if (used != NULL) *used = g_block_mgr.used_block_num;
    if (total != NULL) *total = BLOCK_NUM_TOTAL;
    if(!print_detail){
        return;
    }
    for(uint16_t i = 0; i < BLOCK_NUM_TOTAL; i++){
        if(g_block_mgr.block_index[i].type != BLOCK_TYPE_NONE){
            rt_kprintf("Block %d: Type=%d, ID=%.11s, Channel=%d\n", i, g_block_mgr.block_index[i].type, g_block_mgr.block_index[i].id, g_block_mgr.block_index[i].channel);
        }
    }
}

flash_status read_block_item_by_block_num(uint16_t block_num, block_index_item_t *item){
    if (!g_mgr_inited || block_num >= BLOCK_NUM_TOTAL || item == NULL) {
        return FLASH_ERROR_PARAM;
    }
    *item = g_block_mgr.block_index[block_num];
    return FLASH_OK;
}

INIT_COMPONENT_EXPORT(flash_block_manager_init);