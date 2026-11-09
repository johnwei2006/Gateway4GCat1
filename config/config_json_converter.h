#ifndef CONFIG_JSON_CONVERTER_H
#define CONFIG_JSON_CONVERTER_H
#include "../utils/cJSON.h"
#include "config_manager.h"
#include "../gateway_manager/gateway_info.h"


bool config_gateway_info_to_json(gateway_info_t *gateway_info, config_gateway_serial_t *gateway_config, lte_info_t* lte_info, char *value);

// =====================  JSON ↔ 结构体 互转接口  =====================
/**
 * @brief JSON字符串 → 网关结构体
 * @param json_str 外部传入的网关JSON串
 * @param gateway_cfg 输出网关结构体（需上层提前分配）
 * @return 成功返回true，失败返回false
 */
bool config_json_to_gateway(const char* json_str, config_gateway_serial_t* gateway_cfg);

/**
 * @brief 网关结构体 → JSON字符串
 * @param gateway_cfg 网关结构体
 * @param out_json_buf 输出JSON缓冲区（需上层提前分配）
 * @param buf_len 缓冲区长度
 * @return 成功返回true，失败返回false
 */
bool config_gateway_to_json(const config_gateway_serial_t* gateway_cfg, char* out_json_buf);

/**
 * @brief JSON字符串 → 节点结构体
 * @param json_str 外部传入的节点JSON串
 * @param node_cfg 输出节点结构体（需上层提前分配）
 * @return 成功返回true，失败返回false
 */
bool config_json_to_node(const char* json_str, config_node_list_t* node_cfg) ;

/**
 * @brief 节点结构体 → JSON字符串
 * @param node_cfg 节点结构体
 * @param out_json_buf 输出JSON缓冲区（需上层提前分配）
 * @param buf_len 缓冲区长度
 * @return 成功返回true，失败返回false
 */
bool config_node_to_json(const config_node_list_t* node_cfg, char* out_json_buf);

/**
 * @brief JSON字符串 → 传感器结构体
 * @param json_str 外部传入的传感器JSON串
 * @param sensor_cfg 输出传感器结构体（需上层提前分配）
 * @return 成功返回true，失败返回false
 */
bool config_json_to_sensor(const char* json_str, config_sensor_list_t* sensor_cfg);

/**
 * @brief 传感器结构体 → JSON字符串
 * @param sensor_cfg 传感器结构体
 * @param out_json_buf 输出JSON缓冲区（需上层提前分配）
 * @param buf_len 缓冲区长度
 * @return 成功返回true，失败返回false
 */
bool config_sensor_to_json(const config_sensor_list_t* sensor_cfg, char* out_json_buf);

/**
 * @brief JSON字符串 → 设备结构体
 * @param json_str 外部传入的设备JSON串
 * @param device_cfg 输出设备结构体（需上层提前分配）
 * @return 成功返回true，失败返回false
 */
bool config_json_to_device(const char* json_str, config_device_list_t* device_cfg);

/**
 * @brief 设备结构体 → JSON字符串
 * @param device_cfg 设备结构体
 * @param out_json_buf 输出JSON缓冲区（需上层提前分配）
 * @param buf_len 缓冲区长度
 * @return 成功返回true，失败返回false
 */
bool config_device_to_json(const config_device_list_t* device_cfg, char* out_json_buf);
// =====================  内部cJSON内存管理  =====================

bool config_task_list_to_json(const config_gateway_task_list_t* task_list, char* out_json_buf);

bool config_json_to_task_list(const char* json_str, config_gateway_task_list_t* task_list);
/**
 * @brief 安全释放cJSON对象
 * @param json_root 待释放的cJSON二级指针
 */
void config_json_free_root(cJSON** json_root);

static void _config_time_to_str(const uint8_t* config_time, char* out, uint32_t out_len);

static void _str_to_config_time(const char* str, uint8_t* config_time);
#endif
/*
 * @file config_json_converter.h
 * @brief 配置JSON与结构体互转模块头文件
 * @version 1.6.0
 * @date 2026-03-12
 * @note 基于cJSON实现，提供配置的JSON序列化/反序列化能力，包含完整的错误处理
 */

