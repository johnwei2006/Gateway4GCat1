#include "config_json_converter.h"
#include <stdlib.h>
#include "../gateway_manager/gateway_info.h"
#include "../config/config_manager.h"
#include <math.h>
#include <string.h>
#include <rtthread.h>

static void _safe_strcpy(char* dest, const char* src, uint32_t dest_len) {
    if (dest == NULL || src == NULL || dest_len == 0) return;
    strncpy(dest, src, dest_len - 1);
    dest[dest_len - 1] = '\0';
}

void config_json_free_root(cJSON** json_root) {
    if (json_root != NULL && *json_root != NULL) {
        cJSON_Delete(*json_root);
        *json_root = NULL;
    }
}

bool config_gateway_info_to_json(gateway_info_t *gateway_info, config_gateway_serial_t *gateway_config, lte_info_t* lte_info, char *value){
    if(gateway_info == NULL || value == NULL){
        return false;
    }
    cJSON* json_root = cJSON_CreateObject();
    if(json_root == NULL){
        return false;
    }

    cJSON_AddNumberToObject(json_root, "config_version", gateway_config->config_version); // 配置版本，便于未来升级时兼容老版本配置
    cJSON_AddStringToObject(json_root, "gateway_id", (char*)gateway_config->gateway_id);
    cJSON_AddStringToObject(json_root, "project_id", (char*)gateway_config->project_id);
    cJSON_AddStringToObject(json_root, "system_server_url", (char*)gateway_config->system_server_url);
    cJSON_AddNumberToObject(json_root, "system_server_port", gateway_config->system_server_port);

    //cJSON_AddStringToObject(json_root, "business_server_url", (char*)gateway_config->business_server_url);
    cJSON_AddNumberToObject(json_root, "report_interval", gateway_config->report_interval);
    cJSON_AddNumberToObject(json_root, "heartbeat_interval", gateway_config->heartbeat_interval);
		cJSON_AddNumberToObject(json_root, "lora_freq_group", gateway_config->config_lora.loraFreqGroup);
		
    if(lte_info != NULL && lte_info->lte_state != UDP_STATE_UNINIT){
        cJSON_AddNumberToObject(json_root, "lte_state", lte_info->lte_state);
        cJSON_AddStringToObject(json_root, "server_ip", lte_info->server_ip);
        cJSON_AddNumberToObject(json_root, "server_port", lte_info->server_port);
    
        if(UDP_STATE_CONNECTED == lte_info->lte_state){
            cJSON_AddNumberToObject(json_root, "ec20_csq", lte_info->ec20_csq);
            cJSON_AddStringToObject(json_root, "ec20_imei", lte_info->ec20_imei);
            cJSON_AddStringToObject(json_root, "ec20_iccid", lte_info->ec20_iccid);
            cJSON_AddStringToObject(json_root, "ec20_imsi", lte_info->ec20_imsi);
            cJSON_AddStringToObject(json_root, "ec20_operator", lte_info->ec20_operator);
            cJSON_AddStringToObject(json_root, "ec20_gmi", lte_info->ec20_gmi);
            cJSON_AddStringToObject(json_root, "ec20_gmm", lte_info->ec20_gmm);
            cJSON_AddStringToObject(json_root, "ec20_gmr", lte_info->ec20_gmr);
            //cJSON_AddStringToObject(json_root, "lte_time", gateway_info->lte_info->lte_time);
        }
    }

    cJSON_AddNumberToObject(json_root, "memory_heap_total", gateway_info->memory_heap_total);
    cJSON_AddNumberToObject(json_root, "memory_heap_used", gateway_info->memory_heap_used);
    cJSON_AddNumberToObject(json_root, "memory_heap_free", gateway_info->memory_heap_free);
    cJSON_AddNumberToObject(json_root, "flash_total", gateway_info->flash_total);
    cJSON_AddNumberToObject(json_root, "flash_used", gateway_info->flash_used);
    
    //cJSON_AddNumberToObject(json_root, "uplink_message_size", gateway_info->uplink_message_size);
    cJSON_AddNumberToObject(json_root, "battery_voltage", round(gateway_info->battery_voltage * 100) / 100);
    cJSON_AddBoolToObject(json_root, "adapter_exists", gateway_info->adapter_exists);
    cJSON_AddBoolToObject(json_root, "charging_enabled", gateway_info->charging_enabled);

    char out_json_buf[32] = {0};
    uint8_t reboot_time[6] = {0};
    gateway_manager_get_reboot_time(reboot_time);
    _config_time_to_str(reboot_time, out_json_buf, 20);
    cJSON_AddStringToObject(json_root, "latest_reboot_time", out_json_buf);

     _config_time_to_str(gateway_info->rtc_time, out_json_buf, 20);
    cJSON_AddStringToObject(json_root, "rtc_time", out_json_buf);

    snprintf(out_json_buf, sizeof(out_json_buf), "%08X%08X%08X",
             gateway_info->gateway_mcuid[0],
             gateway_info->gateway_mcuid[1],
             gateway_info->gateway_mcuid[2]);
    cJSON_AddStringToObject(json_root, "gateway_mcuid", out_json_buf);

    char* json_str = cJSON_PrintUnformatted(json_root);
    if(json_str == NULL){
        config_json_free_root(&json_root);
        return false;
    }
    _safe_strcpy(value, json_str, strlen(json_str) + 1);
    rt_free(json_str);
    config_json_free_root(&json_root);
    return true;
};

// =====================  网关 JSON ↔ 结构体  =====================
bool config_json_to_gateway(const char* json_str, config_gateway_serial_t* gateway_cfg) {
    if (json_str == NULL || gateway_cfg == NULL) return false;

    // 1. 解析JSON串
    cJSON* json_root = cJSON_Parse(json_str);
    if (json_root == NULL) return false;

    // 2. 提取各字段到结构体
    cJSON* item = NULL;
    item = cJSON_GetObjectItem(json_root, "config_version");
    if (item != NULL && cJSON_IsNumber(item)) gateway_cfg->config_version = item->valueint;

    item = cJSON_GetObjectItem(json_root, "project_id");
    if (item != NULL && cJSON_IsString(item)) _safe_strcpy((char*)gateway_cfg->project_id, item->valuestring, PROJECT_ID_MAX_LENGTH + 1);

    item = cJSON_GetObjectItem(json_root, "gateway_id");
    if (item != NULL && cJSON_IsString(item)) _safe_strcpy((char*)gateway_cfg->gateway_id, item->valuestring, GATEWAY_ID_LENGTH + 1);

    item = cJSON_GetObjectItem(json_root, "system_server_url");
    if (item != NULL && cJSON_IsString(item)) _safe_strcpy((char*)gateway_cfg->system_server_url, item->valuestring, SERVER_URL_MAX_LENGTH + 1);

    item = cJSON_GetObjectItem(json_root, "system_server_port");
    if (item != NULL && cJSON_IsNumber(item)) gateway_cfg->system_server_port = item->valueint;

    item = cJSON_GetObjectItem(json_root, "lora_freq_group");
    if (item != NULL && cJSON_IsNumber(item)) gateway_cfg->config_lora.loraFreqGroup = item->valueint;

    item = cJSON_GetObjectItem(json_root, "report_interval");
    if (item != NULL && cJSON_IsNumber(item)) gateway_cfg->report_interval = item->valueint;

    item = cJSON_GetObjectItem(json_root, "heartbeat_interval");
    if (item != NULL && cJSON_IsNumber(item)) gateway_cfg->heartbeat_interval = item->valueint;
    item = cJSON_GetObjectItem(json_root, "config_time");
    if (item != NULL && cJSON_IsString(item)) 
        _str_to_config_time(item->valuestring,gateway_cfg->config_time);
    
    // 3. 释放cJSON内存
    config_json_free_root(&json_root);
    return true;
}

bool config_gateway_to_json(const config_gateway_serial_t* gateway_cfg, char* out_json_buf) {
    if (gateway_cfg == NULL || out_json_buf == NULL) return false;

    // 1. 创建cJSON对象并填充字段
    cJSON* json_root = cJSON_CreateObject();
    if (json_root == NULL) return false;

    cJSON_AddNumberToObject(json_root, "config_version", gateway_cfg->config_version);
    cJSON_AddStringToObject(json_root, "project_id", (char*)gateway_cfg->project_id);
    cJSON_AddStringToObject(json_root, "gateway_id", (char*)gateway_cfg->gateway_id);
    cJSON_AddStringToObject(json_root, "system_server_url", (char*)gateway_cfg->system_server_url);
    cJSON_AddNumberToObject(json_root, "system_server_port", gateway_cfg->system_server_port);
    //cJSON_AddStringToObject(json_root, "business_server_url", (char*)gateway_cfg->business_server_url);
    cJSON_AddNumberToObject(json_root, "report_interval", gateway_cfg->report_interval);
    cJSON_AddNumberToObject(json_root, "heartbeat_interval", gateway_cfg->heartbeat_interval);
    cJSON_AddNumberToObject(json_root, "lora_freq_group", gateway_cfg->config_lora.loraFreqGroup);
    _config_time_to_str(gateway_cfg->config_time, out_json_buf, 20);
    cJSON_AddStringToObject(json_root, "config_time", out_json_buf);

    // 2. 序列化为JSON字符串并拷贝到输出缓冲区
    char* json_str = cJSON_Print(json_root);
    if (json_str == NULL) {
        config_json_free_root(&json_root);
        return false;
    }

    _safe_strcpy(out_json_buf, json_str, strlen(json_str) + 1);
    rt_free(json_str);
    config_json_free_root(&json_root);
    return true;
}

// =====================  节点 JSON ↔ 结构体  =====================
bool config_json_to_node(const char* json_str, config_node_list_t* node_cfg) {
    if (json_str == NULL || node_cfg == NULL) return false;

    // 1. 初始化结构体
    rt_memset(node_cfg, 0, sizeof(config_node_list_t));
    cJSON* json_root = cJSON_Parse(json_str);
    if (json_root == NULL) return false;

    // 2. 提取节点数组
    cJSON* json_nodes = cJSON_GetObjectItem(json_root, "config_nodes");
    if (json_nodes == NULL || !cJSON_IsArray(json_nodes)) {
        config_json_free_root(&json_root);
        return false;
    }

    // 3. 遍历数组填充结构体
    int node_cnt = cJSON_GetArraySize(json_nodes);
    node_cfg->nodes_count = (node_cnt > MAX_NODES) ? MAX_NODES : node_cnt;
		
    for (int i = 0; i < node_cfg->nodes_count; i++) {
        cJSON* json_node = cJSON_GetArrayItem(json_nodes, i);
        if (json_node == NULL) continue;

        cJSON* item = cJSON_GetObjectItem(json_node, "node_type");
        if (item != NULL && cJSON_IsNumber(item)) 
            node_cfg->config_nodes[i].node_type = item->valueint;

        item = cJSON_GetObjectItem(json_node, "node_id");
        if (item != NULL && cJSON_IsString(item)) 
            _safe_strcpy((char*)node_cfg->config_nodes[i].node_id, item->valuestring, NODE_ID_LENGTH + 1);

        item = cJSON_GetObjectItem(json_node, "report_interval");
        if (item != NULL && cJSON_IsNumber(item)) 
            node_cfg->config_nodes[i].report_interval = item->valueint;
        item = cJSON_GetObjectItem(json_node, "lora_freq_group");
        if (item != NULL && cJSON_IsNumber(item)) 
            node_cfg->config_nodes[i].lora_freq_group = item->valueint;        
				item = cJSON_GetObjectItem(json_root, "config_time");
				if (item != NULL && cJSON_IsString(item)) 
					_str_to_config_time(item->valuestring,node_cfg->config_nodes[i].config_time);
    }
    cJSON* item = cJSON_GetObjectItem(json_root, "config_time");
    if (item != NULL && cJSON_IsString(item)) 		
			_str_to_config_time(item->valuestring,node_cfg->config_time);

    config_json_free_root(&json_root);
    return true;
}

bool config_node_to_json(const config_node_list_t* node_cfg, char* out_json_buf) {
    if (node_cfg == NULL || out_json_buf == NULL) return false;

    // 1. 创建cJSON对象和节点数组
    cJSON* json_root = cJSON_CreateObject();
    cJSON* json_nodes = cJSON_CreateArray();
    if (json_root == NULL || json_nodes == NULL) {
        config_json_free_root(&json_root);
        return false;
    }
    cJSON_AddItemToObject(json_root, "config_nodes", json_nodes);

    // 2. 填充节点数组
    for (uint32_t i = 0; i < node_cfg->nodes_count; i++) {
        cJSON* json_node = cJSON_CreateObject();
        cJSON_AddNumberToObject(json_node, "node_type", node_cfg->config_nodes[i].node_type);
        char node_id_str[NODE_ID_LENGTH + 1];
        memcpy(node_id_str, node_cfg->config_nodes[i].node_id, NODE_ID_LENGTH);
        node_id_str[NODE_ID_LENGTH] = '\0';
        cJSON_AddStringToObject(json_node, "node_id", node_id_str);
        cJSON_AddNumberToObject(json_node, "report_interval", node_cfg->config_nodes[i].report_interval);
        cJSON_AddNumberToObject(json_node, "lora_freq_group", node_cfg->config_nodes[i].lora_freq_group);
        cJSON_AddNumberToObject(json_node, "sensors_count", node_cfg->config_nodes[i].sensors_count);
        cJSON_AddNumberToObject(json_node, "devices_count", node_cfg->config_nodes[i].devices_count);
        cJSON_AddItemToArray(json_nodes, json_node);
    }
    _config_time_to_str(node_cfg->config_time, out_json_buf, 20);
    cJSON_AddStringToObject(json_root, "config_time", out_json_buf);
    // 3. 序列化为JSON字符串
    char* json_str = cJSON_Print(json_root);
    if (json_str == NULL) {
        config_json_free_root(&json_root);
        return false;
    }

    _safe_strcpy(out_json_buf, json_str, strlen(json_str) + 1);
    rt_free(json_str);
    config_json_free_root(&json_root);
    return true;
}

// =====================  传感器 JSON ↔ 结构体  =====================
bool config_json_to_sensor(const char* json_str, config_sensor_list_t* sensor_cfg) {
    if (json_str == NULL || sensor_cfg == NULL) return false;

    // 1. 初始化结构体
    memset(sensor_cfg, 0, sizeof(config_sensor_list_t));
    cJSON* json_root = cJSON_Parse(json_str);
    if (json_root == NULL) return false;

    // 2. 提取节点ID
    cJSON* item = cJSON_GetObjectItem(json_root, "node_id");
    if (item != NULL && cJSON_IsString(item)) 
        _safe_strcpy((char*)sensor_cfg->node_id, item->valuestring, NODE_ID_LENGTH + 1);

    // 3. 提取传感器数组
    cJSON* json_sensors = cJSON_GetObjectItem(json_root, "config_sensors");
    if (json_sensors == NULL || !cJSON_IsArray(json_sensors)) {
        config_json_free_root(&json_root);
        return false;
    }

    // 4. 遍历数组填充结构体
    int sensor_count = cJSON_GetArraySize(json_sensors);
    sensor_cfg->sensor_count = (sensor_count > MAX_CHANNELS) ? MAX_CHANNELS : sensor_count;

    for (int i = 0; i < sensor_cfg->sensor_count; i++) {
        cJSON* json_sensor = cJSON_GetArrayItem(json_sensors, i);
        if (json_sensor == NULL) continue;

        item = cJSON_GetObjectItem(json_sensor, "channel");
        if (item != NULL && cJSON_IsNumber(item)) 
            sensor_cfg->sensors[i].channel = item->valueint;

        item = cJSON_GetObjectItem(json_sensor, "sensor_type");
        if (item != NULL && cJSON_IsNumber(item)) 
            sensor_cfg->sensors[i].sensor_type = item->valueint;

        item = cJSON_GetObjectItem(json_sensor, "sensor_code");
        if (item != NULL && cJSON_IsString(item)) 
            _safe_strcpy((char*)sensor_cfg->sensors[i].sensor_code, item->valuestring, DEVICE_CODE_MAX_LENGTH + 1);

        item = cJSON_GetObjectItem(json_sensor, "read_hex");
        if (item != NULL && cJSON_IsString(item)) 
            _safe_strcpy((char*)sensor_cfg->sensors[i].read_hex, item->valuestring, COMMAND_HEX_MAX_LENGTH + 1);
    }
    item = cJSON_GetObjectItem(json_root, "config_time");
    if (item != NULL && cJSON_IsString(item)) 
        _str_to_config_time(item->valuestring,sensor_cfg->config_time);

    config_json_free_root(&json_root);
    return true;
}

bool config_sensor_to_json(const config_sensor_list_t* sensor_cfg, char* out_json_buf) {
    if (sensor_cfg == NULL || out_json_buf == NULL) return false;

    // 1. 创建cJSON对象和传感器数组
    cJSON* json_root = cJSON_CreateObject();
    cJSON* json_sensors = cJSON_CreateArray();
    if (json_root == NULL || json_sensors == NULL) {
        config_json_free_root(&json_root);
        return false;
    }

    // 2. 填充节点ID和传感器数组
    char node_id_str[NODE_ID_LENGTH + 1];
    memcpy(node_id_str, sensor_cfg->node_id, NODE_ID_LENGTH);
    node_id_str[NODE_ID_LENGTH] = '\0';
    cJSON_AddStringToObject(json_root, "node_id", node_id_str);
    cJSON_AddItemToObject(json_root, "config_sensors", json_sensors);

    for (uint32_t i = 0; i < sensor_cfg->sensor_count; i++) {
        cJSON* json_sensor = cJSON_CreateObject();
        cJSON_AddNumberToObject(json_sensor, "channel", sensor_cfg->sensors[i].channel);
        cJSON_AddNumberToObject(json_sensor, "sensor_type", sensor_cfg->sensors[i].sensor_type);
        char sensor_code_str[DEVICE_CODE_MAX_LENGTH + 1];
        memcpy(sensor_code_str, sensor_cfg->sensors[i].sensor_code, DEVICE_CODE_MAX_LENGTH);
        sensor_code_str[DEVICE_CODE_MAX_LENGTH] = '\0';
        cJSON_AddStringToObject(json_sensor, "sensor_code", sensor_code_str);

        char read_hex_str[COMMAND_HEX_MAX_LENGTH + 1];
        memcpy(read_hex_str, sensor_cfg->sensors[i].read_hex, COMMAND_HEX_MAX_LENGTH);
        read_hex_str[COMMAND_HEX_MAX_LENGTH] = '\0';
        cJSON_AddStringToObject(json_sensor, "read_hex", read_hex_str);
        cJSON_AddItemToArray(json_sensors, json_sensor);
    }
    _config_time_to_str(sensor_cfg->config_time, out_json_buf, 20);
    cJSON_AddStringToObject(json_root, "config_time", out_json_buf);
    // 3. 序列化为JSON字符串
    char* json_str = cJSON_Print(json_root);
    if (json_str == NULL) {
        config_json_free_root(&json_root);
        return false;
    }

    _safe_strcpy(out_json_buf, json_str, strlen(json_str) + 1);
    rt_free(json_str);
    config_json_free_root(&json_root);
    return true;
}

// =====================  设备 JSON ↔ 结构体  =====================
bool config_json_to_device(const char* json_str, config_device_list_t* device_cfg) {
    if (json_str == NULL || device_cfg == NULL) return false;

    // 1. 初始化结构体
    rt_memset(device_cfg, 0, sizeof(config_device_list_t));
    cJSON* json_root = cJSON_Parse(json_str);
    if (json_root == NULL) return false;

    // 2. 提取节点ID
    cJSON* item = cJSON_GetObjectItem(json_root, "node_id");
    if (item != NULL && cJSON_IsString(item)) _safe_strcpy((char*)device_cfg->node_id, item->valuestring, NODE_ID_LENGTH + 1);

    // 3. 提取设备数组
    cJSON* json_devices = cJSON_GetObjectItem(json_root, "config_devices");
    if (json_devices == NULL || !cJSON_IsArray(json_devices)) {
        config_json_free_root(&json_root);
        return false;
    }

    // 4. 遍历数组填充结构体
    int device_count = cJSON_GetArraySize(json_devices);
    device_cfg->device_count = (device_count > MAX_CHANNELS) ? MAX_CHANNELS : device_count;

    for (int i = 0; i < device_cfg->device_count; i++) {
        cJSON* json_device = cJSON_GetArrayItem(json_devices, i);
        if (json_device == NULL) continue;

        item = cJSON_GetObjectItem(json_device, "channel");
        if (item != NULL && cJSON_IsNumber(item)) 
            device_cfg->devices[i].channel = item->valueint;

        item = cJSON_GetObjectItem(json_device, "switch_type");
        if (item != NULL && cJSON_IsNumber(item)) 
            device_cfg->devices[i].switch_type = item->valueint;

        item = cJSON_GetObjectItem(json_device, "device_type");
        if (item != NULL && cJSON_IsNumber(item)) 
            device_cfg->devices[i].device_type = item->valueint;

        item = cJSON_GetObjectItem(json_device, "device_code");
        if (item != NULL && cJSON_IsString(item)) 
            _safe_strcpy((char*)device_cfg->devices[i].device_code, item->valuestring, DEVICE_CODE_MAX_LENGTH + 1);

        item = cJSON_GetObjectItem(json_device, "on_hex");
        if (item != NULL && cJSON_IsString(item)) 
            _safe_strcpy((char*)device_cfg->devices[i].on_hex, item->valuestring, COMMAND_HEX_MAX_LENGTH + 1);

        item = cJSON_GetObjectItem(json_device, "off_hex");
        if (item != NULL && cJSON_IsString(item)) 
            _safe_strcpy((char*)device_cfg->devices[i].off_hex, item->valuestring, COMMAND_HEX_MAX_LENGTH + 1);

        item = cJSON_GetObjectItem(json_device, "set_hex");
        if (item != NULL && cJSON_IsString(item)) 
            _safe_strcpy((char*)device_cfg->devices[i].set_hex, item->valuestring, COMMAND_HEX_MAX_LENGTH + 1);
    }
    item = cJSON_GetObjectItem(json_root, "config_time");
    if (item != NULL && cJSON_IsString(item)) 
        _str_to_config_time(item->valuestring,device_cfg->config_time);
    
    config_json_free_root(&json_root);
    return true;
}

bool config_device_to_json(const config_device_list_t* device_cfg, char* out_json_buf) {
    if (device_cfg == NULL || out_json_buf == NULL) return false;

    // 1. 创建cJSON对象和设备数组
    cJSON* json_root = cJSON_CreateObject();
    cJSON* json_devices = cJSON_CreateArray();
    if (json_root == NULL || json_devices == NULL) {
        config_json_free_root(&json_root);
        return false;
    }

    // 2. 填充节点ID和设备数组
    char node_id_str[NODE_ID_LENGTH + 1];
    memcpy(node_id_str, device_cfg->node_id, NODE_ID_LENGTH);
    node_id_str[NODE_ID_LENGTH] = '\0';
    cJSON_AddStringToObject(json_root, "node_id", node_id_str);
    cJSON_AddItemToObject(json_root, "config_devices", json_devices);

    for (uint32_t i = 0; i < device_cfg->device_count; i++) {
        cJSON* json_device = cJSON_CreateObject();
        cJSON_AddNumberToObject(json_device, "channel", device_cfg->devices[i].channel);
        cJSON_AddNumberToObject(json_device, "switch_type", device_cfg->devices[i].switch_type);
        cJSON_AddNumberToObject(json_device, "device_type", device_cfg->devices[i].device_type);
        char device_code_str[DEVICE_CODE_MAX_LENGTH + 1];
        memcpy(device_code_str, device_cfg->devices[i].device_code, DEVICE_CODE_MAX_LENGTH);
        device_code_str[DEVICE_CODE_MAX_LENGTH] = '\0';
        cJSON_AddStringToObject(json_device, "device_code", device_code_str);

        char on_hex_str[COMMAND_HEX_MAX_LENGTH + 1];
        memcpy(on_hex_str, device_cfg->devices[i].on_hex, COMMAND_HEX_MAX_LENGTH);
        on_hex_str[COMMAND_HEX_MAX_LENGTH] = '\0';
        cJSON_AddStringToObject(json_device, "on_hex", on_hex_str);

        char off_hex_str[COMMAND_HEX_MAX_LENGTH + 1];
        memcpy(off_hex_str, device_cfg->devices[i].off_hex, COMMAND_HEX_MAX_LENGTH);
        off_hex_str[COMMAND_HEX_MAX_LENGTH] = '\0';
        cJSON_AddStringToObject(json_device, "off_hex", off_hex_str);

        char set_hex_str[COMMAND_HEX_MAX_LENGTH + 1];
        memcpy(set_hex_str, device_cfg->devices[i].set_hex, COMMAND_HEX_MAX_LENGTH);
        set_hex_str[COMMAND_HEX_MAX_LENGTH] = '\0';
        cJSON_AddStringToObject(json_device, "set_hex", set_hex_str);
        cJSON_AddItemToArray(json_devices, json_device);
    }
    _config_time_to_str(device_cfg->config_time, out_json_buf, 20);
    cJSON_AddStringToObject(json_root, "config_time", out_json_buf);
    // 3. 序列化为JSON字符串
    char* json_str = cJSON_Print(json_root);
    if (json_str == NULL) {
        config_json_free_root(&json_root);
        return false;
    }

    _safe_strcpy(out_json_buf, json_str, strlen(json_str) + 1);
    rt_free(json_str);
    config_json_free_root(&json_root);
    return true;
}
bool config_task_list_to_json(const config_gateway_task_list_t* task_list, char* out_json_buf){
    if (task_list == NULL || out_json_buf == NULL) return false;
    
    // 1. 创建cJSON对象和任务数组
    cJSON* json_root = cJSON_CreateObject();
    cJSON* json_tasks = cJSON_CreateArray();
    if (json_root == NULL || json_tasks == NULL) {
        config_json_free_root(&json_root);
        return false;
    }
    cJSON_AddItemToObject(json_root, "config_tasks", json_tasks);
    
    // 2. 填充节点ID和任务数组
    for (uint8_t i = 0; i < task_list->task_count; i++) {
        cJSON* json_task = cJSON_CreateObject();
        cJSON_AddNumberToObject(json_task, "task_id", task_list->config_tasks[i].task_id);
        cJSON_AddNumberToObject(json_task, "trigger_type", task_list->config_tasks[i].trigger_type);
        cJSON_AddNumberToObject(json_task, "enabled", task_list->config_tasks[i].enabled);
        char trigger_time_str[8] = {0};
        snprintf(trigger_time_str, sizeof(trigger_time_str), "%02X%02X%02X", 
                 task_list->config_tasks[i].trigger_time[0], 
                 task_list->config_tasks[i].trigger_time[1], 
                 task_list->config_tasks[i].trigger_time[2]);
        cJSON_AddStringToObject(json_task, "trigger_time", trigger_time_str);
        cJSON_AddNumberToObject(json_task, "trigger_size", task_list->config_tasks[i].trigger_size);
         cJSON* json_triggers = cJSON_CreateArray();
        for (int j = 0; j < task_list->config_tasks[i].trigger_size; j++) {
            cJSON* json_trigger = cJSON_CreateObject();
            cJSON_AddNumberToObject(json_trigger, "join_type", task_list->config_tasks[i].triggers[j].join_type);
            char trigger_node_id[NODE_ID_LENGTH + 1] = {0};
            rt_memcpy(trigger_node_id, task_list->config_tasks[i].triggers[j].node_id, NODE_ID_LENGTH);
            cJSON_AddStringToObject(json_trigger, "node_id", trigger_node_id);
            // cJSON_AddStringToObject(json_trigger, "node_id", (const char*)task_list->config_tasks[i].triggers[j].node_id);
            cJSON_AddNumberToObject(json_trigger, "channel", task_list->config_tasks[i].triggers[j].channel);
            cJSON_AddNumberToObject(json_trigger, "compare_type", task_list->config_tasks[i].triggers[j].compare_type);
            cJSON_AddNumberToObject(json_trigger, "value", task_list->config_tasks[i].triggers[j].value);
            cJSON_AddItemToArray(json_triggers, json_trigger);
        }
        cJSON_AddItemToObject(json_task, "triggers", json_triggers);
        
        cJSON_AddNumberToObject(json_task, "action_size", task_list->config_tasks[i].action_size);
        
        // 创建 actions 数组
        cJSON* json_actions = cJSON_CreateArray();
        for (int j = 0; j < task_list->config_tasks[i].action_size; j++) {
            cJSON* json_action = cJSON_CreateObject();
            char action_node_id[NODE_ID_LENGTH + 1] = {0};
            rt_memcpy(action_node_id, task_list->config_tasks[i].actions[j].node_id, NODE_ID_LENGTH);
            cJSON_AddStringToObject(json_action, "node_id", action_node_id);
            cJSON_AddNumberToObject(json_action, "channel", task_list->config_tasks[i].actions[j].channel);
            cJSON_AddNumberToObject(json_action, "action", task_list->config_tasks[i].actions[j].action);
            cJSON_AddNumberToObject(json_action, "value", task_list->config_tasks[i].actions[j].value);
            cJSON_AddItemToArray(json_actions, json_action);
        }
        cJSON_AddItemToObject(json_task, "actions", json_actions);
        cJSON_AddItemToArray(json_tasks, json_task);
    }

    // 3. 序列化为JSON字符串
    char* json_str = cJSON_Print(json_root);
    if (json_str == NULL) {
        config_json_free_root(&json_root);
        return false;
    }

    _safe_strcpy(out_json_buf, json_str, strlen(json_str) + 1);
    rt_free(json_str);
    config_json_free_root(&json_root);
};

bool config_json_to_task_list(const char* json_str, config_gateway_task_list_t* task_list){
    if (json_str == NULL || task_list == NULL) 
        return false;
    // 1. 初始化结构体
    rt_memset(task_list, 0, sizeof(config_gateway_task_list_t));
    cJSON* json_root = cJSON_Parse(json_str);
    if (json_root == NULL){
        return false;
    } else {
        // 2. 解析任务数组
    cJSON* json_tasks = cJSON_GetObjectItem(json_root, "config_tasks");
    if (json_tasks == NULL) {
        config_json_free_root(&json_root);
        return false;
    }
    int task_count = cJSON_GetArraySize(json_tasks);
    task_list->task_count = task_count;
    cJSON* item = NULL;
    for (int i = 0; i < task_count; i++) {
        cJSON* json_task = cJSON_GetArrayItem(json_tasks, i);
        if (json_task == NULL) {
            config_json_free_root(&json_root);
            return false;
        }
        // 解析task_id
        item = cJSON_GetObjectItem(json_task, "task_id");
        if (item != NULL && cJSON_IsNumber(item))   
            task_list->config_tasks[i].task_id = item->valueint;
        item = cJSON_GetObjectItem(json_task, "trigger_type");
        if (item != NULL && cJSON_IsNumber(item)) 
            task_list->config_tasks[i].trigger_type = item->valueint;
        item = cJSON_GetObjectItem(json_task, "enabled");
        if (item != NULL && cJSON_IsNumber(item)) 
            task_list->config_tasks[i].enabled = item->valueint;
        item = cJSON_GetObjectItem(json_task, "trigger_time");
        if (item != NULL && cJSON_IsString(item)) {
            sscanf(item->valuestring, "%02X%02X%02X", 
                   &task_list->config_tasks[i].trigger_time[0], 
                   &task_list->config_tasks[i].trigger_time[1], 
                   &task_list->config_tasks[i].trigger_time[2]);
        }

        item = cJSON_GetObjectItem(json_task, "triggers");
        if (item != NULL && cJSON_IsArray(item)){
            int trigger_count = cJSON_GetArraySize(item);
            task_list->config_tasks[i].trigger_size = trigger_count;
            //task_list->config_tasks[i].triggers = (config_gateway_task_trigger_t*)rt_malloc(trigger_count * sizeof(config_gateway_task_trigger_t));
            for (int j = 0; j < trigger_count; j++) {
                cJSON* json_trigger = cJSON_GetArrayItem(item, j);
                if (json_trigger == NULL) {
                    config_json_free_root(&json_root);
                    return false;
                }
                cJSON* trigger_item = cJSON_GetObjectItem(json_trigger, "join_type");
                if (trigger_item != NULL && cJSON_IsNumber(trigger_item)){
                    task_list->config_tasks[i].triggers[j].join_type = trigger_item->valueint;
                }
                trigger_item = cJSON_GetObjectItem(json_trigger, "node_id");
                if (trigger_item != NULL && cJSON_IsString(trigger_item)){
                    _safe_strcpy((char*)task_list->config_tasks[i].triggers[j].node_id, trigger_item->valuestring, NODE_ID_LENGTH + 1);
                }
                trigger_item = cJSON_GetObjectItem(json_trigger, "channel");
                if (trigger_item != NULL && cJSON_IsNumber(trigger_item)){
                    task_list->config_tasks[i].triggers[j].channel = trigger_item->valueint;
                }
                trigger_item = cJSON_GetObjectItem(json_trigger, "compare_type");
                if (trigger_item != NULL && cJSON_IsNumber(trigger_item)){
                    task_list->config_tasks[i].triggers[j].compare_type = trigger_item->valueint;
                }
                trigger_item = cJSON_GetObjectItem(json_trigger, "value");
                if (trigger_item != NULL && cJSON_IsNumber(trigger_item)){
                    task_list->config_tasks[i].triggers[j].value = trigger_item->valueint;
                }
            }
        }
        item = cJSON_GetObjectItem(json_task, "actions");
        if (item != NULL && cJSON_IsArray(item)){
            int action_count = cJSON_GetArraySize(item);
            task_list->config_tasks[i].action_size = action_count;
            //task_list->config_tasks[i].actions = (config_gateway_task_action_t*)rt_malloc(action_count * sizeof(config_gateway_task_action_t));
            for (int j = 0; j < action_count; j++) {
                cJSON* json_action = cJSON_GetArrayItem(item, j);
                if (json_action == NULL) {
                    config_json_free_root(&json_root);
                    return false;
                }
                cJSON* action_item = cJSON_GetObjectItem(json_action, "node_id");
                if (action_item != NULL && cJSON_IsString(action_item)){
                    _safe_strcpy((char*)task_list->config_tasks[i].actions[j].node_id, action_item->valuestring, NODE_ID_LENGTH + 1 );
                }
                action_item = cJSON_GetObjectItem(json_action, "channel");
                if (action_item != NULL && cJSON_IsNumber(action_item)){
                    task_list->config_tasks[i].actions[j].channel = action_item->valueint;
                }
                action_item = cJSON_GetObjectItem(json_action, "action");
                if (action_item != NULL && cJSON_IsNumber(action_item)){
                    task_list->config_tasks[i].actions[j].action = action_item->valueint;
                }                    
                action_item = cJSON_GetObjectItem(json_action, "value");
                if (action_item != NULL && cJSON_IsNumber(action_item)){
                    task_list->config_tasks[i].actions[j].value = action_item->valueint;
                }
            }
        }
    }
    config_json_free_root(&json_root);
    return true;
	}
}
static void _config_time_to_str(const uint8_t* config_time, char* out, uint32_t out_len) {
    if (config_time == NULL || out == NULL || out_len < 20) return;
    snprintf(out, out_len, "20%02d-%02d-%02d %02d:%02d:%02d",
             config_time[0], config_time[1], config_time[2],
             config_time[3], config_time[4], config_time[5]);
}

static void _str_to_config_time(const char* str, uint8_t* config_time) {
    // 空指针直接返回
    if (str == NULL || config_time == NULL)
    {
        return;
    }

    // 1. 清空原有数据，避免残留脏值
    memset(config_time, 0, 6);

    // 2. 校验字符串必须恰好12位纯数字
    size_t str_len = strlen(str);
    if (str_len != 12)
    {
        return;
    }
    for (size_t i = 0; i < 12; i++)
    {
        if (!isdigit((unsigned char)str[i]))
        {
            return;
        }
    }

    // 3. 解析12位数字
    unsigned int y, mo, d, h, mi, s;
    if (sscanf(str, "%2u%2u%2u%2u%2u%2u", &y, &mo, &d, &h, &mi, &s) != 6)
    {
        return;
    }

    // 4. 严格校验各时间字段合法范围
    if (y > 99) return;
    if (mo < 1 || mo > 12) return;
    if (d < 1 || d > 31) return;
    if (h > 23) return;
    if (mi > 59) return;
    if (s > 59) return;

    // 全部校验通过后再赋值
    config_time[0] = (uint8_t)y;
    config_time[1] = (uint8_t)mo;
    config_time[2] = (uint8_t)d;
    config_time[3] = (uint8_t)h;
    config_time[4] = (uint8_t)mi;
    config_time[5] = (uint8_t)s;
}