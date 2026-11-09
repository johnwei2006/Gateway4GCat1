#include <stdint.h>
#include <string.h>

// ========================== 配置宏定义 ==========================
// Flash地址配置 (根据实际硬件调整)
#define FLASH_BASE_ADDR          0x08000000      // Flash起始地址
#define BOOTLOADER_SIZE         (32 * 1024)     // Bootloader占用32KB
#define APP_A_START_ADDR        (FLASH_BASE_ADDR + BOOTLOADER_SIZE)  // APP A起始地址
#define APP_B_START_ADDR        (APP_A_START_ADDR + 192 * 1024)     // APP B起始地址 (192KB)
#define UPGRADE_FLAG_ADDR       (FLASH_BASE_ADDR + 0x70000)         // 升级标志存储地址 (Flash末尾)

// 分区状态定义
#define PARTITION_A             0
#define PARTITION_B             1

// 升级状态定义
#define UPGRADE_IDLE            0               // 无升级
#define UPGRADE_STARTED         1               // 升级开始（已接收固件信息）
#define UPGRADE_DOWNLOADING     2               // 固件分块下载中
#define UPGRADE_FINISHED        3               // 升级完成
#define UPGRADE_FAILED          4               // 升级失败

// 固件校验相关
#define CRC32_POLYNOMIAL        0xEDB88320      // CRC32标准多项式

// OTA配置
#define OTA_BLOCK_SIZE          1024            // 单次接收分块大小 (字节)
#define OTA_MAX_RETRY           3               // 分块接收最大重试次数

// ========================== 数据结构定义 ==========================
typedef struct {
    uint8_t active_partition;   // 当前激活的分区 (0:A, 1:B)
    uint8_t upgrade_status;     // 升级状态
    uint32_t firmware_size;     // 固件总大小
    uint32_t firmware_crc;      // 固件CRC校验值
    uint32_t downloaded_size;   // 已下载字节数 (OTA断点续传用)
    uint8_t reserved[12];       // 预留空间，凑齐Flash扇区大小
} upgrade_flag_t;

// OTA固件信息结构体 (从服务器获取)
typedef struct {
    uint32_t total_size;        // 固件总大小
    uint32_t crc32;             // 固件CRC32值
    uint16_t block_count;       // 总分块数
} ota_firmware_info_t;

// ========================== 函数声明 ==========================
// Flash操作函数 (需根据硬件平台实现)
void flash_read(uint32_t addr, uint8_t* buf, uint32_t len);
void flash_erase(uint32_t addr, uint32_t len);
void flash_write(uint32_t addr, uint8_t* buf, uint32_t len);

// 固件校验函数
uint32_t calculate_crc32(const uint8_t* data, uint32_t length);
int verify_firmware(uint32_t addr, uint32_t size, uint32_t crc);

// 升级标志操作函数
void read_upgrade_flag(upgrade_flag_t* flag);
void write_upgrade_flag(upgrade_flag_t* flag);

// 分区切换与应用启动函数
void switch_to_backup_partition(void);
void app_entry(uint32_t app_address);
void jump_to_app(void);

// OTA核心函数 (需适配具体OTA协议：如MQTT/HTTP/TCP/UDP)
int ota_get_firmware_info(ota_firmware_info_t* info);  // 获取固件信息
int ota_receive_block(uint16_t block_num, uint8_t* buf, uint32_t buf_len);  // 接收单个分块
void ota_send_ack(uint16_t block_num, int result);     // 发送分块接收确认

// OTA升级流程函数
int ota_start_upgrade(void);
int ota_download_firmware(void);
int ota_finish_upgrade(void);
void ota_abort_upgrade(void);

// ========================== 函数实现 ==========================
/**
 * @brief 读取升级标志
 * @param flag 升级标志结构体指针
 */
void read_upgrade_flag(upgrade_flag_t* flag) {
    if (flag == NULL) return;
    flash_read(UPGRADE_FLAG_ADDR, (uint8_t*)flag, sizeof(upgrade_flag_t));
}

/**
 * @brief 写入升级标志
 * @param flag 升级标志结构体指针
 */
void write_upgrade_flag(upgrade_flag_t* flag) {
    if (flag == NULL) return;
    // Flash写入前先擦除 (需确保地址和长度对齐到扇区)
    flash_erase(UPGRADE_FLAG_ADDR, sizeof(upgrade_flag_t));
    flash_write(UPGRADE_FLAG_ADDR, (uint8_t*)flag, sizeof(upgrade_flag_t));
}

/**
 * @brief 计算CRC32校验值
 * @param data 数据指针
 * @param length 数据长度
 * @return CRC32校验值
 */
uint32_t calculate_crc32(const uint8_t* data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? CRC32_POLYNOMIAL : 0);
        }
    }
    return ~crc;
}

/**
 * @brief 验证固件有效性
 * @param addr 固件起始地址
 * @param size 固件大小
 * @param crc 预期CRC校验值
 * @return 1:校验通过, 0:校验失败
 */
int verify_firmware(uint32_t addr, uint32_t size, uint32_t crc) {
    // 1. 检查地址和大小合法性
    if (addr < APP_A_START_ADDR || size == 0 || size > (192 * 1024)) {
        return 0;
    }

    // 2. 读取固件并计算CRC
    uint8_t* firmware_buf = (uint8_t*)addr;  // 直接指向Flash地址 (需确保可直接访问)
    uint32_t calculated_crc = calculate_crc32(firmware_buf, size);

    // 3. 对比CRC值
    return (calculated_crc == crc) ? 1 : 0;
}

/**
 * @brief 切换到备份分区
 */
void switch_to_backup_partition(void) {
    upgrade_flag_t flag;
    read_upgrade_flag(&flag);

    // 切换激活分区 (A<->B)
    flag.active_partition = (flag.active_partition == PARTITION_A) ? PARTITION_B : PARTITION_A;
    flag.upgrade_status = UPGRADE_IDLE;  // 重置升级状态

    // 写入新的分区标志
    write_upgrade_flag(&flag);

    // 重新尝试启动备份分区
    jump_to_app();
}

/**
 * @brief 跳转到应用程序
 * @param app_address 应用起始地址
 */
void app_entry(uint32_t app_address) {
    // 函数指针类型定义 (匹配ARM Cortex-M系列中断向量表)
    typedef void (*pFunction)(void);
    pFunction app_main;

    // 1. 检查栈顶地址有效性 (简单校验)
    uint32_t stack_top = *((volatile uint32_t*)app_address);
    if ((stack_top & 0x2FFE0000) != 0x20000000) {  // 假设RAM地址范围:0x20000000-0x200FFFFF
        return;
    }

    // 2. 设置应用入口地址 (中断向量表第二项是复位函数地址)
    app_main = (pFunction)*((volatile uint32_t*)(app_address + 4));

    // 3. 关闭中断 (可选，根据实际需求)
    __disable_irq();

    // 4. 设置MSP (主栈指针)
    __set_MSP(stack_top);

    // 5. 跳转到应用程序
    app_main();
}

/**
 * @brief 启动应用程序 (核心函数)
 */
void jump_to_app(void) {
    upgrade_flag_t flag;
    read_upgrade_flag(&flag);

    // 确定当前激活分区的地址
    uint32_t app_address = (flag.active_partition == PARTITION_A) ? APP_A_START_ADDR : APP_B_START_ADDR;

    // 验证固件有效性
    if (verify_firmware(app_address, flag.firmware_size, flag.firmware_crc)) {
        app_entry(app_address);  // 校验通过，跳转到应用
    } else {
        switch_to_backup_partition();  // 校验失败，切换到备份分区
    }
}

/**
 * @brief 启动OTA升级 (获取固件信息，初始化升级状态)
 * @return 0:成功, -1:失败
 */
int ota_start_upgrade(void) {
    upgrade_flag_t flag;
    ota_firmware_info_t fw_info;

    // 1. 读取当前升级状态，避免重复升级
    read_upgrade_flag(&flag);
    if (flag.upgrade_status == UPGRADE_DOWNLOADING || flag.upgrade_status == UPGRADE_STARTED) {
        return 0;  // 已有升级在进行，直接继续
    }

    // 2. 从OTA服务器获取固件信息 (需适配具体协议)
    if (ota_get_firmware_info(&fw_info) != 0) {
        return -1;
    }

    // 3. 校验固件信息合法性
    if (fw_info.total_size == 0 || fw_info.total_size > (192 * 1024)) {
        return -1;
    }

    // 4. 确定待升级分区 (非当前激活分区)
    uint32_t upgrade_partition_addr = (flag.active_partition == PARTITION_A) ? APP_B_START_ADDR : APP_A_START_ADDR;
    
    // 5. 擦除待升级分区 (首次升级时执行)
    flash_erase(upgrade_partition_addr, fw_info.total_size);

    // 6. 更新升级标志
    flag.upgrade_status = UPGRADE_STARTED;
    flag.firmware_size = fw_info.total_size;
    flag.firmware_crc = fw_info.crc32;
    flag.downloaded_size = 0;  // 初始化已下载大小
    write_upgrade_flag(&flag);

    return 0;
}

/**
 * @brief 下载OTA固件 (分块接收并写入Flash)
 * @return 0:成功, -1:失败
 */
int ota_download_firmware(void) {
    upgrade_flag_t flag;
    ota_firmware_info_t fw_info;
    uint8_t block_buf[OTA_BLOCK_SIZE];
    int retry_count;

    // 1. 读取升级状态和固件信息
    read_upgrade_flag(&flag);
    if (flag.upgrade_status != UPGRADE_STARTED && flag.upgrade_status != UPGRADE_DOWNLOADING) {
        return -1;  // 未启动升级，直接返回
    }
    if (ota_get_firmware_info(&fw_info) != 0) {
        return -1;
    }

    // 2. 确定待升级分区地址
    uint32_t upgrade_partition_addr = (flag.active_partition == PARTITION_A) ? APP_B_START_ADDR : APP_A_START_ADDR;

    // 3. 计算起始分块号 (断点续传)
    uint16_t start_block = flag.downloaded_size / OTA_BLOCK_SIZE;
    uint16_t total_blocks = (fw_info.total_size + OTA_BLOCK_SIZE - 1) / OTA_BLOCK_SIZE;

    // 4. 标记为下载中状态
    flag.upgrade_status = UPGRADE_DOWNLOADING;
    write_upgrade_flag(&flag);

    // 5. 分块接收固件
    for (uint16_t block_num = start_block; block_num < total_blocks; block_num++) {
        retry_count = 0;
        memset(block_buf, 0, OTA_BLOCK_SIZE);

        // 重试接收当前分块
        while (retry_count < OTA_MAX_RETRY) {
            if (ota_receive_block(block_num, block_buf, OTA_BLOCK_SIZE) == 0) {
                break;  // 接收成功，退出重试
            }
            retry_count++;
        }
        if (retry_count >= OTA_MAX_RETRY) {
            ota_abort_upgrade();
            return -1;  // 分块接收失败，终止升级
        }

        // 计算当前分块写入地址
        uint32_t write_addr = upgrade_partition_addr + (block_num * OTA_BLOCK_SIZE);
        // 计算当前分块实际大小 (最后一块可能不足OTA_BLOCK_SIZE)
        uint32_t block_size = (block_num == total_blocks - 1) ? (fw_info.total_size % OTA_BLOCK_SIZE) : OTA_BLOCK_SIZE;
        if (block_size == 0) block_size = OTA_BLOCK_SIZE;

        // 写入Flash
        flash_write(write_addr, block_buf, block_size);

        // 更新已下载大小并保存状态 (防止断电丢失)
        flag.downloaded_size += block_size;
        write_upgrade_flag(&flag);

        // 发送接收确认
        ota_send_ack(block_num, 0);
    }

    return 0;
}

/**
 * @brief 完成OTA升级 (校验并切换分区)
 * @return 0:成功, -1:失败
 */
int ota_finish_upgrade(void) {
    upgrade_flag_t flag;
    read_upgrade_flag(&flag);

    if (flag.upgrade_status != UPGRADE_DOWNLOADING) {
        return -1;  // 未完成下载，直接返回失败
    }

    // 1. 检查已下载大小是否完整
    if (flag.downloaded_size != flag.firmware_size) {
        ota_abort_upgrade();
        return -1;
    }

    // 2. 获取待升级分区地址
    uint32_t upgrade_partition_addr = (flag.active_partition == PARTITION_A) ? APP_B_START_ADDR : APP_A_START_ADDR;

    // 3. 验证写入的固件
    if (!verify_firmware(upgrade_partition_addr, flag.firmware_size, flag.firmware_crc)) {
        flag.upgrade_status = UPGRADE_FAILED;
        write_upgrade_flag(&flag);
        return -1;
    }

    // 4. 更新升级标志，切换激活分区
    flag.upgrade_status = UPGRADE_FINISHED;
    flag.active_partition = (flag.active_partition == PARTITION_A) ? PARTITION_B : PARTITION_A;
    flag.downloaded_size = 0;  // 重置下载进度
    write_upgrade_flag(&flag);

    return 0;
}

/**
 * @brief 终止OTA升级 (恢复状态)
 */
void ota_abort_upgrade(void) {
    upgrade_flag_t flag;
    read_upgrade_flag(&flag);

    flag.upgrade_status = UPGRADE_FAILED;
    flag.downloaded_size = 0;
    write_upgrade_flag(&flag);
}

/**
 * @brief OTA底层函数示例 (需根据实际OTA协议实现，如MQTT/HTTP)
 * @note 以下为占位实现，需替换为真实的OTA通信逻辑
 */
int ota_get_firmware_info(ota_firmware_info_t* info) {
    // 示例：从服务器获取固件信息 (实际需实现网络请求)
    if (info == NULL) return -1;
    // 模拟返回固件信息
    info->total_size = 128 * 1024;    // 128KB固件
    info->crc32 = 0x12345678;        // 模拟CRC值
    info->block_count = info->total_size / OTA_BLOCK_SIZE;
    return 0;
}

int ota_receive_block(uint16_t block_num, uint8_t* buf, uint32_t buf_len) {
    // 示例：接收指定分块数据 (实际需实现网络接收)
    if (buf == NULL || buf_len < OTA_BLOCK_SIZE) return -1;
    // 模拟接收数据 (实际需从网络读取)
    memset(buf, 0xAA, buf_len);
    return 0;
}

void ota_send_ack(uint16_t block_num, int result) {
    // 示例：发送分块接收确认 (实际需实现网络发送)
    (void)block_num;
    (void)result;
}

/**
 * @brief Bootloader主函数
 */
int main(void) {
    // 1. 初始化硬件 (串口、网络、Flash、时钟等，需根据实际平台实现)
    // hardware_init();
    // network_init();  // OTA必备：初始化网络 (Wi-Fi/4G/蓝牙等)

    // 2. 检查OTA升级请求 (如通过网络指令、按键触发等)
    int ota_request = 1;  // 模拟存在OTA升级请求
    if (ota_request) {
        // 执行OTA升级流程
        if (ota_start_upgrade() == 0) {          // 启动升级
            if (ota_download_firmware() == 0) {  // 下载固件
                if (ota_finish_upgrade() == 0) { // 完成升级
                    // 升级成功，重启后运行新固件
                    // system_reset();
                }
            }
        }
    }

    // 3. 启动应用程序
    jump_to_app();

    // 4. 如果启动失败，进入错误处理
    while (1) {
        // 闪烁LED等错误提示
    }
}