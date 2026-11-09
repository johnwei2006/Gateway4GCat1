#include "config_manager.h"
#include <rtthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "../console/console.h"
#include "config_json_converter.h"
#include "../flash_external/flash_block.h"
#include "../flash_external/abstract_flash_external.h"
#include "../utils/utils.h"
#include "../lte/lte_app.h"
#include "../gateway_manager/gateway_manager.h"
#include "../gateway_manager/thread_manager.h"

#define BASE_CONFIG_ADDRESS     0x0000u

static rt_mutex_t mutex_config 		= RT_NULL;
static bool is_config_initialized 				= false;
static config_gateway_serial_t 	config_gateway_serial_default;
static config_gateway_t 				config_gateway_default;

rt_bool_t config_init(void){
   if(is_config_initialized){
      return RT_TRUE;
   }
   mutex_config = rt_mutex_create("mutex_config", RT_IPC_FLAG_FIFO);
   if(mutex_config == RT_NULL)
   {
			rt_kprintf("create config mutex failed.\n");
			return false;
   }
   is_config_initialized = true;
   return RT_TRUE;
};

flash_status read_config_gateway_serial(config_gateway_serial_t *gateway_config){
    if (gateway_config == NULL) {
        return FLASH_ERROR_PARAM;
    }
    memset(gateway_config, 0, sizeof(config_gateway_serial_t));

    uint16_t block_num = block_find_by_type(BLOCK_TYPE_GATEWAY_CONFIG, 1);
    if (block_num == 0xFFFF) {
        return FLASH_ERROR_NOT_FOUND;
    }
    return block_read(block_num, gateway_config, sizeof(config_gateway_serial_t));   
};

static flash_status write_config_gateway_serial(config_gateway_serial_t *gateway_config){
    if (gateway_config == NULL || gateway_config->gateway_id == NULL) {
        return FLASH_ERROR_PARAM;
    }

    uint16_t block_num = block_find(BLOCK_TYPE_GATEWAY_CONFIG, gateway_config->gateway_id, -1);
    if (block_num == 0xFFFF) {
        // 分配新块
        block_num = block_alloc(BLOCK_TYPE_GATEWAY_CONFIG, gateway_config->gateway_id, -1, GATEWAY_CONFIG_BLOCK_NUM);
        if (block_num == 0xFFFF) {
            return FLASH_ERROR_FULL;
        }
    }
    // assign flash block failed
    if (block_num != 0) {
        return FLASH_ERROR_ACCUPIED;
    }

    return block_write(GATEWAY_CONFIG_BLOCK_NUM, gateway_config, sizeof(config_gateway_serial_t));  
};

flash_status update_config_gateway(config_gateway_serial_t *gateway_config){
    if(gateway_config == NULL || gateway_config->gateway_id == NULL){
        return FLASH_ERROR_PARAM;
    }
    config_gateway_serial_t gateway_config_existed;
    flash_status status = read_config_gateway_serial(&gateway_config_existed);
    if(status == FLASH_OK){//update existing gateway_config
        if(rt_strcmp(gateway_config->gateway_id, gateway_config_existed.gateway_id) != 0){
            rt_kprintf("write_config_by_key: gateway_id mismatch! %s != %s\n", gateway_config->gateway_id, gateway_config_existed.gateway_id);
            return FLASH_ERROR_INVALID;
        }
        if(gateway_config->config_version == 0){
            gateway_config->config_version = gateway_config_existed.config_version;
        }            
        /*if(gateway_config->business_server_url == RT_NULL){
            strcpy(gateway_config->business_server_url, gateway_config_existed.business_server_url);
        }*/
       
        if(gateway_config->system_server_url == RT_NULL){
            strcpy(gateway_config->system_server_url, gateway_config_existed.system_server_url);
        }
        if(gateway_config->system_server_port == 0){
            gateway_config->system_server_port = gateway_config_existed.system_server_port;
        }        
        if(gateway_config->project_id == RT_NULL){
            strcpy(gateway_config->project_id, gateway_config_existed.project_id);
        }
        if(gateway_config->report_interval == 0){
            gateway_config->report_interval = gateway_config_existed.report_interval;
        }
        if(gateway_config->heartbeat_interval == 0){
            gateway_config->heartbeat_interval = gateway_config_existed.heartbeat_interval;
        }
				if(gateway_config->config_lora.loraFreqGroup == 0){
            gateway_config->config_lora.loraFreqGroup = gateway_config_existed.config_lora.loraFreqGroup;
        }
        // preserve existing node configs in gateway_config_existed
        gateway_config->nodes_count = gateway_config_existed.nodes_count;
        for(uint8_t i = 0; i < gateway_config->nodes_count; i++){
            gateway_config->node_block_numbers[i] = gateway_config_existed.node_block_numbers[i];
        }
        //gateway_config->config_lora = gateway_config_existed.config_lora;
    }
    return write_config_gateway_serial(gateway_config);
};

flash_status read_config_node_serial(config_node_serial_t *config_node, const char* node_id){
    if (config_node == NULL || node_id == NULL) {
        return FLASH_ERROR_PARAM;       
    }

    uint16_t block_num = block_find(BLOCK_TYPE_NODE_CONFIG, node_id, -1);
    if (block_num == 0xFFFF) {
        return FLASH_ERROR_NOT_FOUND;
    }

    return block_read(block_num, config_node, sizeof(config_node_serial_t));  
};
static flash_status add_or_update_config_node_serial(config_node_serial_t *config_node, uint16_t *block_num){
    if (config_node == NULL) {
        return FLASH_ERROR_PARAM;
    }

    // 先查找是否已存在该传感器块
    *block_num = block_find(BLOCK_TYPE_NODE_CONFIG, config_node->node_id, -1);
    if (*block_num == 0xFFFF) {
        // 分配新块
        *block_num = block_alloc(BLOCK_TYPE_NODE_CONFIG, config_node->node_id, -1, -1);
        if (*block_num == 0xFFFF) {
            return FLASH_ERROR_FULL;
        }
        return block_write(*block_num, config_node, sizeof(config_node_serial_t));  
    } else {
        // 更新已存在的块
        config_node_serial_t config_node_existed = {0};
        flash_status status = block_read(*block_num, &config_node_existed, sizeof(config_node_serial_t));
        if(status != FLASH_OK){
            rt_kprintf("read_config_by_key: read node config failed!\n");
            return RT_FALSE;
        }
        // update config_node_existed
        config_node->sensors_count = config_node_existed.sensors_count > MAX_CHANNELS ? MAX_CHANNELS : config_node_existed.sensors_count;
        config_node->devices_count = config_node_existed.devices_count > MAX_CHANNELS ? MAX_CHANNELS : config_node_existed.devices_count;
        for(uint8_t i = 0; i < config_node->sensors_count; i++){
            config_node->sensor_block_numbers[i] = config_node_existed.sensor_block_numbers[i];
        }
        for(uint8_t i = 0; i < config_node->devices_count; i++){
            config_node->device_block_numbers[i] = config_node_existed.device_block_numbers[i];
        }
        return block_write(*block_num, config_node, sizeof(config_node_serial_t));  
    }
};
static flash_status delete_config_node_all_sensor_device_by_block(uint16_t block_num);

flash_status update_config_node(config_node_list_t *config_node_list){
    if(config_node_list == NULL){
        return FLASH_ERROR_PARAM;
    }
    config_gateway_serial_t gateway_config;
    flash_status status = read_config_gateway_serial(&gateway_config);
    if(status != FLASH_OK){
        rt_kprintf("read_config_by_key: read gateway config failed!\n");
        return RT_FALSE;
    }
    // 1. check and delete nodes that are not in config_node_list

    uint16_t block_nums_to_free[MAX_NODES];
    uint8_t  block_nums_to_free_count = 0;

    for (uint8_t i = 0; i < gateway_config.nodes_count; i++){
        bool found = false;
        for(uint8_t j = 0; j < config_node_list->nodes_count; j++){
            config_node_serial_t config_node = {0};
            status = block_read(gateway_config.node_block_numbers[i], &config_node, sizeof(config_node_serial_t));
            if(status != FLASH_OK){
                rt_kprintf("read_config_by_key: read node %d config failed!\n", i);
                continue;
            }            
            if(rt_memcmp(config_node.node_id, config_node_list->config_nodes[j].node_id, NODE_ID_LENGTH) == 0){
                found = true;
                break;
            }
        }
        if(!found){
            block_nums_to_free[block_nums_to_free_count++] = gateway_config.node_block_numbers[i];
        }
    }
    gateway_config.nodes_count = 0;
  
    uint16_t block_num = 0xFFFF;
    // 2. add or update nodes in config_node_list
    for (uint8_t i = 0; i < config_node_list->nodes_count; i++) {
        status = add_or_update_config_node_serial(&config_node_list->config_nodes[i], &block_num);
        if(status != FLASH_OK){
            rt_kprintf("write_config_by_key: write node config failed!\n");
            return RT_FALSE;
        }
        gateway_config.nodes_count++;
        gateway_config.node_block_numbers[gateway_config.nodes_count - 1] = block_num;
    }
    for(uint8_t i = gateway_config.nodes_count; i < MAX_NODES; i++){
        gateway_config.node_block_numbers[i] = 0;
    }
    // 3. update gateway config
    rt_memcpy(gateway_config.config_time, config_node_list->config_time, sizeof(gateway_config.config_time));
    status = write_config_gateway_serial(&gateway_config);
    if(status != FLASH_OK){
        rt_kprintf("write_config_by_key: commit gateway config failed!\n");
        return RT_FALSE;
    }

    for(uint8_t i = 0; i < block_nums_to_free_count; i++){
        delete_config_node_all_sensor_device_by_block(block_nums_to_free[i]);
        block_free(block_nums_to_free[i]);
    }

    return FLASH_OK;
};

flash_status query_config_node_list(config_node_list_t *config_node_list){
    if(config_node_list == NULL){
        return FLASH_ERROR_PARAM;
    }
    config_gateway_serial_t gateway_config = {0};
    flash_status status = read_config_gateway_serial(&gateway_config);
    if(status != FLASH_OK){
        rt_kprintf("read_config_by_key: read gateway config failed!\n");
        return RT_FALSE;
    }
    config_node_list->nodes_count = gateway_config.nodes_count;
    for(uint8_t i = 0; i < gateway_config.nodes_count; i++){
        config_node_serial_t config_node = {0};
        status = block_read(gateway_config.node_block_numbers[i], &config_node, sizeof(config_node_serial_t)); 
        if(status != FLASH_OK){
            rt_kprintf("read_config_by_key: read node %d config failed!\n", i);
            return RT_FALSE;
        }
        config_node_list->config_nodes[i].node_type = config_node.node_type;
        memcpy(config_node_list->config_nodes[i].node_id, config_node.node_id, NODE_ID_LENGTH);  
        config_node_list->config_nodes[i].report_interval = config_node.report_interval;
        config_node_list->config_nodes[i].sensors_count = config_node.sensors_count;
        config_node_list->config_nodes[i].devices_count = config_node.devices_count;
    }
    rt_memcpy(config_node_list->config_time, gateway_config.config_time, sizeof(gateway_config.config_time));
    return status;
};

static flash_status delete_config_node_all_sensor_device_by_block(uint16_t block_num){
    config_node_serial_t node_serial;
    flash_status ret = block_read(block_num, &node_serial, sizeof(config_node_serial_t));
    if (ret != FLASH_OK) {
        return ret;
    }
    return delete_config_node_all_sensor_device(node_serial.node_id);
};

static flash_status delete_config_node_all_sensor_device(const char* node_id){
    if(node_id == NULL){
        return FLASH_ERROR_PARAM;
    }
    config_node_serial_t node_serial;
    flash_status ret = read_config_node_serial(&node_serial, node_id);
    if (ret != FLASH_OK) {
        return ret;
    } 

    uint16_t deleted_block_size = block_delete_by_type_and_id(BLOCK_TYPE_NODE_SENSOR, node_id);
    rt_kprintf("delete %d sensor blocks\n", deleted_block_size);

    node_serial.sensors_count = 0;
    for(uint16_t i = 0; i < MAX_CHANNELS; i++){
        node_serial.sensor_block_numbers[i] = 0;
    }

    deleted_block_size = block_delete_by_type_and_id(BLOCK_TYPE_NODE_DEVICE, node_id);
    rt_kprintf("delete %d device blocks\n", deleted_block_size);
    
    node_serial.devices_count = 0;
    for(uint16_t i = 0; i < MAX_CHANNELS; i++){
        node_serial.device_block_numbers[i] = 0;
    }

    uint16_t block_num = block_find(BLOCK_TYPE_NODE_CONFIG, node_id, -1);
    if (block_num == 0xFFFF) {
        return FLASH_ERROR_NOT_FOUND;
    }
    return block_write(block_num, &node_serial, sizeof(config_node_serial_t));
};

static flash_status read_config_sensor(config_sensor_t *config_sensor, const char* node_id, uint16_t block_num){
    if (node_id == NULL || block_num == 0xFFFF) {
        return FLASH_ERROR_PARAM;
    }
    // 查找块号
    // uint16_t block_num = block_find(BLOCK_TYPE_NODE_SENSOR, node_id, channel);
    if (block_num == 0xFFFF) {
        return FLASH_ERROR_NOT_FOUND;
    }

    // 读取块数据
    return block_read(block_num, config_sensor, sizeof(config_sensor_t));    
};
flash_status query_config_sensor_by_channel(config_sensor_t *config_sensor, const char* node_id, uint8_t channel){
    if (node_id == NULL || channel == 0xFF) {
        return FLASH_ERROR_PARAM;
    }
    uint16_t block_num = block_find(BLOCK_TYPE_NODE_SENSOR, node_id, channel);
    if (block_num == 0xFFFF) {
        return FLASH_ERROR_NOT_FOUND;
    }
    return read_config_sensor(config_sensor, node_id, block_num);
};
flash_status add_or_update_config_sensor(config_sensor_t *config_sensor, const char* node_id, uint16_t *block_num){
    if (config_sensor == NULL || node_id == NULL) {
        return FLASH_ERROR_PARAM;
    }

    // 先查找是否已存在该传感器块
    *block_num = block_find(BLOCK_TYPE_NODE_SENSOR, node_id, config_sensor->channel);
    if (*block_num == 0xFFFF) {
        // 分配新块
        *block_num = block_alloc(BLOCK_TYPE_NODE_SENSOR, node_id, config_sensor->channel, -1);
        if (*block_num == 0xFFFF) {
            return FLASH_ERROR_FULL;
        }
    }
    // 写入传感器数据
    return block_write(*block_num, config_sensor, sizeof(config_sensor_t));    
};
static flash_status delete_config_sensor(config_sensor_t *config_sensor, const char* node_id){
    if (config_sensor == NULL || node_id == NULL) {
        return FLASH_ERROR_PARAM;
    }    
    // 先查找是否已存在该传感器块
    uint16_t block_num = block_find(BLOCK_TYPE_NODE_SENSOR, node_id, config_sensor->channel);
    if (block_num != 0xFFFF) {
        flash_status status = block_free(block_num);
        if(status != FLASH_OK){
            return status;
        }
        config_node_serial_t node_serial;
        status = read_config_node_serial(&node_serial, node_id);
        remove_item_by_value(node_serial.sensor_block_numbers, node_serial.sensors_count, block_num);
        node_serial.sensors_count -= 1;
        uint16_t node_block_num = block_find(BLOCK_TYPE_NODE_CONFIG, node_serial.node_id, -1);
        if (node_block_num == 0xFFFF) {
            return FLASH_ERROR_NOT_FOUND;
        }
        status = block_write(node_block_num, &node_serial, sizeof(config_node_serial_t));
        return status;
    } 
};

flash_status update_config_sensor_list(config_sensor_list_t *config_sensor_list){
    config_node_serial_t node_config = {0};
    flash_status status = read_config_node_serial(&node_config, config_sensor_list->node_id);
    if(status != FLASH_OK){
        rt_kprintf("read_config_node_serial: read node config failed!\n");
        return RT_FALSE;
    }

    uint16_t block_nums_to_free[MAX_CHANNELS];
    uint8_t  block_nums_to_free_count = 0;

    for(uint8_t i = 0; i < node_config.sensors_count; i++){
        bool found = false;
        block_index_item_t block_item;
        if (read_block_item_by_block_num(node_config.sensor_block_numbers[i], &block_item) == FLASH_OK) {
            for(uint8_t j = 0; j < config_sensor_list->sensor_count; j++){
                if(config_sensor_list->sensors[j].channel == block_item.channel){
                    found = true;
                    break;
                }
            }
        }
        if(!found){
            block_nums_to_free[block_nums_to_free_count++] = node_config.sensor_block_numbers[i];
        }
    }
    node_config.sensors_count = 0;

    uint16_t block_num = 0;
    for (uint8_t i = 0; i < config_sensor_list->sensor_count; i++) {
        status = add_or_update_config_sensor(&config_sensor_list->sensors[i], config_sensor_list->node_id, &block_num);
        if(status != FLASH_OK){
            rt_kprintf("add_or_update_config_sensor: write sensor config failed!\n");
            return RT_FALSE;
        }
        node_config.sensors_count++;
        node_config.sensor_block_numbers[node_config.sensors_count - 1] = block_num;
    }

    uint16_t node_block_num = block_find(BLOCK_TYPE_NODE_CONFIG, node_config.node_id, -1);
    if (node_block_num == 0xFFFF) {
        rt_kprintf("update_config_sensor_list: node block not found!\n");
        return FLASH_ERROR_NOT_FOUND;
    }
    status = block_write(node_block_num, &node_config, sizeof(config_node_serial_t));
    if(status != FLASH_OK){
        rt_kprintf("update_config_sensor_list: write node config failed!\n");
        return RT_FALSE;
    }

    for(uint8_t i = 0; i < block_nums_to_free_count; i++){
        block_free(block_nums_to_free[i]);
    }

    return FLASH_OK;
};

flash_status query_config_sensor_list(config_sensor_list_t *config_sensor_list){
    if(config_sensor_list == NULL || config_sensor_list->node_id == NULL){
        return FLASH_ERROR_PARAM;
    }
    config_node_serial_t node_config = {0};
    flash_status status = read_config_node_serial(&node_config, config_sensor_list->node_id);
    if(status != FLASH_OK){
        rt_kprintf("read_config_by_key: read node config failed!\n");
        return RT_FALSE;
    }
    config_sensor_list->sensor_count = node_config.sensors_count;
    for (uint8_t i = 0; i < config_sensor_list->sensor_count; i++) {
        status = read_config_sensor(&config_sensor_list->sensors[i], config_sensor_list->node_id, node_config.sensor_block_numbers[i]);
        if(status != FLASH_OK){
            rt_kprintf("read_config_by_key: read sensor config failed!\n");
            return RT_FALSE;
        }
    }
    rt_memcpy(config_sensor_list->config_time, node_config.config_time, sizeof(node_config.config_time));
    return FLASH_OK;
};
flash_status read_config_device(config_device_t *config_device, const char* node_id, uint16_t block_num){
    if (node_id == NULL || block_num == 0xFFFF) {
        return FLASH_ERROR_PARAM;
    }

    // 查找块号
    //uint16_t block_num = block_find(BLOCK_TYPE_NODE_DEVICE, node_id, channel);
    if (block_num == 0xFFFF) {
        return FLASH_ERROR_NOT_FOUND;
    }

    // 读取块数据
    return block_read(block_num, config_device, sizeof(config_device_t));   
};
flash_status query_config_device_by_channel(config_device_t *config_device, const char* node_id, uint8_t channel){
    if (node_id == NULL || channel == 0xFF) {
        return FLASH_ERROR_PARAM;
    }
    uint16_t block_num = block_find(BLOCK_TYPE_NODE_DEVICE, node_id, channel);
    if (block_num == 0xFFFF) {
        return FLASH_ERROR_NOT_FOUND;
    }
    return read_config_device(config_device, node_id, block_num);
};
flash_status add_or_update_config_device(config_device_t *config_device, const char* node_id, uint16_t *block_num){
    if (config_device == NULL || node_id == NULL) {
        return FLASH_ERROR_PARAM;
    }

    // 先查找是否已存在该设备配置块
    *block_num = block_find(BLOCK_TYPE_NODE_DEVICE, node_id, config_device->channel);
    if (*block_num == 0xFFFF) {
        // 分配新块
        *block_num = block_alloc(BLOCK_TYPE_NODE_DEVICE, node_id, config_device->channel, -1);
        if (*block_num == 0xFFFF) {
            return FLASH_ERROR_FULL;
        }      
    }
    return block_write(*block_num, config_device, sizeof(config_device_t));        
};
flash_status delete_config_device(config_device_t *config_device, const char* node_id){
    if (config_device == NULL || node_id == NULL) {
        return FLASH_ERROR_PARAM;
    }    
    // 先查找是否已存在该传感器块
    uint16_t block_num = block_find(BLOCK_TYPE_NODE_DEVICE, node_id, config_device->channel);
    if (block_num != 0xFFFF) {
        flash_status status = block_free(block_num);
        if(status != FLASH_OK){
            return status;
        }
        config_node_serial_t node_serial;
        status = read_config_node_serial(&node_serial, node_id);
        remove_item_by_value(node_serial.device_block_numbers, node_serial.devices_count, block_num);
        node_serial.devices_count -= 1;
        uint16_t node_block_num = block_find(BLOCK_TYPE_NODE_CONFIG, node_serial.node_id, -1);
        if (node_block_num == 0xFFFF) {
            return FLASH_ERROR_NOT_FOUND;
        }
        status = block_write(node_block_num, &node_serial, sizeof(config_node_serial_t));
        return status;
    }     
};

flash_status update_config_device_list(config_device_list_t *config_device_list){
    config_node_serial_t node_config = {0};
    flash_status status = read_config_node_serial(&node_config, config_device_list->node_id);
    if(status != FLASH_OK){
        rt_kprintf("read_config_by_key: read node config failed!\n");
        return RT_FALSE;
    }

    uint16_t block_nums_to_free[MAX_CHANNELS];
    uint8_t  block_nums_to_free_count = 0;

    for(uint8_t i = 0; i < node_config.devices_count; i++){
        bool found = false;
        block_index_item_t block_item;
        if (read_block_item_by_block_num(node_config.device_block_numbers[i], &block_item) == FLASH_OK) {
            for(uint8_t j = 0; j < config_device_list->device_count; j++){
                if(config_device_list->devices[j].channel == block_item.channel){
                    found = true;
                    break;
                }
            }
        }
        if(!found){
            block_nums_to_free[block_nums_to_free_count++] = node_config.device_block_numbers[i];
        }
    }
    node_config.devices_count = 0;

    uint16_t block_num = 0;
    for (uint8_t i = 0; i < config_device_list->device_count; i++) {
        status = add_or_update_config_device(&config_device_list->devices[i], config_device_list->node_id, &block_num);
        if(status != FLASH_OK){
            rt_kprintf("write_config_by_key: write device config failed!\n");
            return RT_FALSE;
        }
        node_config.devices_count++;
        node_config.device_block_numbers[node_config.devices_count - 1] = block_num;
    }

    uint16_t node_block_num = block_find(BLOCK_TYPE_NODE_CONFIG, node_config.node_id, -1);
    if (node_block_num == 0xFFFF) {
        rt_kprintf("update_config_device_list: node block not found!\n");
        return FLASH_ERROR_NOT_FOUND;
    }
    status = block_write(node_block_num, &node_config, sizeof(config_node_serial_t));
    if(status != FLASH_OK){
        rt_kprintf("update_config_device_list: write node config failed!\n");
        return RT_FALSE;
    }

    for(uint8_t i = 0; i < block_nums_to_free_count; i++){
        block_free(block_nums_to_free[i]);
    }

    return FLASH_OK;
};

flash_status query_config_device_list(config_device_list_t *config_device_list){
    if(config_device_list == NULL || config_device_list->node_id == NULL){
        return FLASH_ERROR_PARAM;
    }
    config_node_serial_t node_config = {0};
    flash_status status = read_config_node_serial(&node_config, config_device_list->node_id);
    if(status != FLASH_OK){
        rt_kprintf("read_config_by_key: read node config failed!\n");
        return RT_FALSE;
    }
    for(uint8_t i = 0; i < node_config.devices_count; i++){
        status = read_config_device(&config_device_list->devices[i], config_device_list->node_id, node_config.device_block_numbers[i]);
        if(status != FLASH_OK){
            rt_kprintf("read_config_by_key: read device config failed!\n");
            return RT_FALSE;
        }
    }
    config_device_list->device_count = node_config.devices_count;
    rt_memcpy(config_device_list->config_time, node_config.config_time, sizeof(node_config.config_time));
    return FLASH_OK;
};

flash_status add_or_update_config_task_list(config_gateway_task_list_t *config_task_list){
    if(config_task_list == NULL){
        return FLASH_ERROR_PARAM;
    }
    uint16_t task_count = config_task_list->task_count > MAX_TASKS_COUNT ? MAX_TASKS_COUNT : config_task_list->task_count;

    for(uint8_t i = 0; i < task_count; i++){
        flash_status status = add_or_update_config_task(&config_task_list->config_tasks[i]);
        if(status != FLASH_OK){
            rt_kprintf("add_or_update_config_task_list: write task %d config failed!\n", i);
            continue;
        }
    }
    return FLASH_OK;
};
flash_status query_config_task_list(config_gateway_task_list_t *config_task_list){
    if(config_task_list == NULL){
        return FLASH_ERROR_PARAM;
    }
    config_task_list->task_count = block_size_by_type(BLOCK_TYPE_TASK);
    if(config_task_list->task_count == 0xFFFF){
        return FLASH_ERROR_NOT_FOUND;
    }
    for(uint8_t i = 0; i < config_task_list->task_count; i++){
        uint16_t block_num = block_find_by_type(BLOCK_TYPE_TASK, i + 1);
        if(block_num == 0xFFFF){
            rt_kprintf("query_config_task_list: task %d config not found!\n", i);
            continue;
        }
        flash_status status = block_read(block_num, &config_task_list->config_tasks[i], sizeof(config_gateway_task_t));
        if(status != FLASH_OK){
            rt_kprintf("query_config_task_list: read task %d config failed!\n", i);
            continue;
        }
    }
	return FLASH_OK;
};

flash_status delete_config_task(config_gateway_task_t *config_task){
    if(config_task == NULL || config_task->task_id == 0){
        return FLASH_ERROR_PARAM;
    } 
    uint16_t block_num = block_find(BLOCK_TYPE_TASK, DEFAULT_GATEWAY_TASK_ID, config_task->task_id);
    if (block_num == 0xFFFF) {
        return FLASH_ERROR_NOT_FOUND;
    }
    flash_status status = block_free(block_num);
    if(status != FLASH_OK){
        return status;
    }
};
flash_status add_or_update_config_task(config_gateway_task_t *config_task){
    if(config_task == NULL){
        return FLASH_ERROR_PARAM;
    }
    if(config_task->task_id == 0){
        return FLASH_ERROR_PARAM;
    }

    uint16_t block_num = block_find(BLOCK_TYPE_TASK, DEFAULT_GATEWAY_TASK_ID, config_task->task_id);
    if (block_num == 0xFFFF) {
        // 分配新块
        block_num = block_alloc(BLOCK_TYPE_TASK, DEFAULT_GATEWAY_TASK_ID, config_task->task_id, -1);
        if (block_num == 0xFFFF) {
            return FLASH_ERROR_FULL;
        }      
    }
    return block_write(block_num, config_task, sizeof(config_gateway_task_t));        
};

/* config console command list
#RD GATEWAY_INFO   
#RD GATEWAY_CONFIG
#ST GATEWAY_CONFIG $CONFIG_JSON$
#RD CONFIG_NODE
#ST CONFIG_NODE $CONFIG_JSON$
#RD CONFIG_SENSOR.$NODE_ID$
#RD CONFIG_SENSOR.$NODE_ID$.$CHANNEL$
#ST CONFIG_SENSOR.$NODE_ID$ $CONFIG_JSON$
#ST CONFIG_SENSOR.$NODE_ID$.$CHANNEL$ $CONFIG_JSON$
#RD CONFIG_DEVICE.$NODE_ID$
#RD CONFIG_DEVICE.$NODE_ID$.$CHANNEL$
#ST CONFIG_DEVICE.$NODE_ID$ $CONFIG_JSON$
#ST CONFIG_DEVICE.$NODE_ID$.$CHANNEL$ $CONFIG_JSON$
*/
bool config_manager_read_config(char* key, char* value, uint32_t value_len){
    if(key == RT_NULL){
        rt_kprintf("read_config_by_key: key is NULL!\n");
        return RT_FALSE;
    }

    //1. GATEWAY INFO QUERY
    if(strncasecmp(key, "GATEWAY_INFO", rt_strlen("GATEWAY_INFO")) == 0){
        config_gateway_serial_t gateway_config;
        flash_status status = read_config_gateway_serial(&gateway_config);
        if(status != FLASH_OK){
            rt_kprintf("read_config_by_key: read gateway config failed!\n");
            return RT_FALSE;
        }        

        gateway_info_t gateway_info = {0};
        gateway_manager_get_gateway_info(&gateway_info);

        lte_info_t lte_info = {0};
        get_lte_info(&lte_info);

        config_gateway_info_to_json(&gateway_info, &gateway_config, &lte_info, value);
        return RT_TRUE;
    //2. read GATEWAY CONFIG
    } else if(strncasecmp(key, "GATEWAY_CONFIG", rt_strlen("GATEWAY_CONFIG")) == 0){
        config_gateway_serial_t gateway_config;
        flash_status status = read_config_gateway_serial(&gateway_config);
        if(status != FLASH_OK){
            rt_kprintf("read_config_by_key: read gateway config failed!\n");
            return RT_FALSE;
        }

        config_gateway_to_json(&gateway_config, value);
        //rt_kprintf("read_config_by_key: gateway config json: %s\n", value);
        return RT_TRUE;
    //3. read NODE CONFIG
    } else if(strncasecmp(key, "CONFIG_NODE", rt_strlen("CONFIG_NODE")) == 0){
        config_gateway_serial_t gateway_config;
        flash_status status = read_config_gateway_serial(&gateway_config);
        if(status != FLASH_OK){
            rt_kprintf("read_config_by_key: read gateway config failed!\n");
            return RT_FALSE;
        }

        config_node_list_t node_config_list = {0};
				memset(node_config_list.config_nodes, 0, MAX_NODES * sizeof(config_node_serial_t));
        node_config_list.nodes_count = gateway_config.nodes_count;

        
        if (gateway_config.nodes_count == 0 || gateway_config.nodes_count > MAX_NODES) {
            rt_kprintf("read_config_by_key: nodes count is invalid (0 or > %d)!\n", MAX_NODES);
            return RT_FALSE;
        }

        // 3. 循环读取FLASH并赋值（数组长度足够，不会覆盖前一个元素）
        for (uint8_t i = 0; i < gateway_config.nodes_count; i++) {
            config_node_serial_t config_node = {0};
            flash_status status = block_read(gateway_config.node_block_numbers[i], &config_node, sizeof(config_node_serial_t)); 
            if (status != FLASH_OK) {
                rt_kprintf("read_config_by_key: read node %d config failed!\n", i);
                return RT_FALSE;
            }
            
            memcpy(node_config_list.config_nodes[i].node_id, config_node.node_id, NODE_ID_LENGTH);
            node_config_list.config_nodes[i].node_type = config_node.node_type;
            node_config_list.config_nodes[i].report_interval = config_node.report_interval;
            node_config_list.config_nodes[i].lora_freq_group = config_node.lora_freq_group;
            rt_memcpy(node_config_list.config_nodes[i].config_time, config_node.config_time, sizeof(config_node.config_time));
            rt_memcpy(node_config_list.config_time, config_node.config_time, sizeof(config_node.config_time));
            node_config_list.config_nodes[i].sensors_count = config_node.sensors_count;
            node_config_list.config_nodes[i].devices_count = config_node.devices_count;
        }
        config_node_to_json(&node_config_list, value);
        return RT_TRUE;
    //4. read SENSOR CONFIG
    } else if(strncasecmp(key, "CONFIG_SENSOR", rt_strlen("CONFIG_SENSOR")) == 0){
        char *node_id = RT_NULL; 
        char *sub_key = strtok(key, COMMAND_KEY_SPLIT);
        while(sub_key != RT_NULL){
            node_id = sub_key;
            sub_key = strtok(NULL, COMMAND_KEY_SPLIT);
        }

        config_node_serial_t config_node = {0};
        flash_status status = read_config_node_serial(&config_node, node_id);
        if(status != FLASH_OK){
            rt_kprintf("read_config_by_key: read node config failed!\n");
            return RT_FALSE;
        }
        config_sensor_list_t *config_sensor_list = rt_calloc(1, sizeof(config_sensor_list_t));
        if (config_sensor_list == RT_NULL) {
            rt_kprintf("read_config_by_key: alloc config_sensor_list failed!\n");
            return RT_FALSE;
        }
        config_sensor_list->sensor_count = config_node.sensors_count;
        memcpy(config_sensor_list->node_id, node_id, NODE_ID_LENGTH);
        //config_sensor_list->node_id[NODE_ID_LENGTH] = '\0';     
        block_index_item_t block_item = {0};
        config_sensor_t config_sensor = {0};
        for(uint8_t i = 0; i < config_node.sensors_count; i++){
            status = read_block_item_by_block_num(config_node.sensor_block_numbers[i], &block_item);
            if(status != FLASH_OK){
                rt_kprintf("read_config_by_key: read sensor block item failed!\n");
                rt_free(config_sensor_list);
                return RT_FALSE;
            }
            status = read_config_sensor(&config_sensor, node_id, block_item.block_num);
            if(status != FLASH_OK){
                rt_kprintf("read_config_by_key: read sensor config failed!\n");
                rt_free(config_sensor_list);
                return RT_FALSE;
            }
            memcpy(&config_sensor_list->sensors[i], &config_sensor, sizeof(config_sensor_t));
        }
        memcpy(config_sensor_list->config_time, config_node.config_time, sizeof(config_node.config_time));
        
        config_sensor_to_json(config_sensor_list, value);
        rt_free(config_sensor_list);
        return RT_TRUE;
    //5. read DEVICE CONFIG
    } else if(strncasecmp(key, "CONFIG_DEVICE", rt_strlen("CONFIG_DEVICE")) == 0){
        char *node_id = RT_NULL; 
        char *sub_key = strtok(key, COMMAND_KEY_SPLIT);
        while(sub_key != RT_NULL){
            node_id = sub_key;
            sub_key = strtok(NULL, COMMAND_KEY_SPLIT);
        }

        config_node_serial_t config_node = {0};
        flash_status status = read_config_node_serial(&config_node, node_id);
        if(status != FLASH_OK){
            rt_kprintf("read_config_by_key: read node config failed!\n");
            return RT_FALSE;
        }
        config_device_list_t *config_device_list = rt_calloc(1, sizeof(config_device_list_t));
        if (config_device_list == RT_NULL) {
            rt_kprintf("read_config_by_key: alloc config_device_list failed!\n");
            return RT_FALSE;
        }
        config_device_list->device_count = config_node.devices_count;
        memcpy(config_device_list->node_id, node_id, NODE_ID_LENGTH);
        block_index_item_t block_item = {0};
        config_device_t config_device = {0};
        for(uint8_t i = 0; i < config_node.devices_count; i++){
            status = read_block_item_by_block_num(config_node.device_block_numbers[i], &block_item);
            if(status != FLASH_OK){
                rt_kprintf("read_config_by_key: read device block item failed!\n");
                rt_free(config_device_list);
                return RT_FALSE;
            }
            status = read_config_device(&config_device, node_id, block_item.block_num);
            if(status != FLASH_OK){
                rt_kprintf("read_config_by_key: read device config failed!\n");
                rt_free(config_device_list);
                return RT_FALSE;
            }
            memcpy(&config_device_list->devices[i], &config_device, sizeof(config_device_t));
        }
        memcpy(config_device_list->config_time, config_node.config_time, sizeof(config_node.config_time));
        
        config_device_to_json(config_device_list, value);
        rt_free(config_device_list);
        return RT_TRUE;
    } else if(strncasecmp(key, "FLASH_BLOCK_INFO", rt_strlen("FLASH_BLOCK_INFO")) == 0){
        uint16_t used, total;
        block_print_status(&used, &total, true);
        rt_kprintf("flash block info used/total: %d/%d\n", used, total);
        return RT_TRUE;
    } else if(strncasecmp(key, "THREADS", rt_strlen("THREADS")) == 0){
        print_all_thread_stack();
        return RT_TRUE;
    } else if(strncasecmp(key, "RTC", rt_strlen("RTC")) == 0){
        rtc_print_time();
        return RT_TRUE;
    } else if(strncasecmp(key, "TASKS", rt_strlen("TASKS")) == 0){
        config_gateway_task_list_t *config_task_list =(config_gateway_task_list_t*)rt_malloc(sizeof(config_gateway_task_list_t));
        flash_status status = query_config_task_list(config_task_list);
        if(status == FLASH_OK){
            config_task_list_to_json(config_task_list, value);
            rt_free(config_task_list);
            return RT_TRUE;
        } else {
            rt_kprintf("read_config_by_key: query task list failed!\n");
            rt_free(config_task_list);
            return RT_FALSE;
        }
    } else if(strncasecmp(key, "STACK", rt_strlen("STACK")) == 0){
        print_all_stack_record();
        return RT_TRUE;
    } else {
         rt_kprintf("read_config_by_key: key %s not found!\n", key);
        return RT_FALSE;       
    }
};

/* config console command list
#RD GATEWAY_INFO   
#RD GATEWAY_CONFIG
#ST GATEWAY_CONFIG $CONFIG_JSON$
#RD CONFIG_NODE
#ST CONFIG_NODE $CONFIG_JSON$
#RD CONFIG_SENSOR.$NODE_ID$
#ST CONFIG_SENSOR.$NODE_ID$ $CONFIG_JSON$
#RD CONFIG_DEVICE.$NODE_ID$
#ST CONFIG_DEVICE.$NODE_ID$ $CONFIG_JSON$
*/
bool config_manager_write_config(char* key, const char* json_value){
    if(key == RT_NULL){
        rt_kprintf("config_manager_write_config: key is NULL!\n");
        return false;
    }
    //to_upper(key);

    flash_status status;
    //1. write GATEWAY CONFIG
    if(rt_strncmp(key, "GATEWAY_CONFIG", rt_strlen("GATEWAY_CONFIG")) == 0){
        config_gateway_serial_t gateway_config = {0};
        config_json_to_gateway(json_value, &gateway_config);
        status = update_config_gateway(&gateway_config);
        if(status != FLASH_OK){
            rt_kprintf("write_config_by_key: update gateway config failed!\n");
            return RT_FALSE;
        }
        return RT_TRUE;
    //2. write NODE CONFIG
    } else if (rt_strncmp(key, "CONFIG_NODE", rt_strlen("CONFIG_NODE")) == 0){
        config_node_list_t *config_node_list = rt_calloc(1, sizeof(config_node_list_t));
        if (config_node_list == RT_NULL) {
            rt_kprintf("write_config_by_key: alloc config_node_list failed!\n");
            return RT_FALSE;
        }
        config_json_to_node(json_value, config_node_list);

        status = update_config_node(config_node_list);
        rt_free(config_node_list);
        if(status != FLASH_OK){
            rt_kprintf("write_config_by_key: update node config failed!\n");
            return RT_FALSE;
        }
        return RT_TRUE;
     //3. write SENSOR CONFIG
    } else if(rt_strncmp(key, "CONFIG_SENSOR", rt_strlen("CONFIG_SENSOR")) == 0){
        config_sensor_list_t *config_sensor_list = rt_calloc(1, sizeof(config_sensor_list_t));
        if (config_sensor_list == RT_NULL) {
            rt_kprintf("write_config_by_key: alloc config_sensor_list failed!\n");
            return RT_FALSE;
        }
        config_json_to_sensor(json_value, config_sensor_list);

        status = update_config_sensor_list(config_sensor_list);
        rt_free(config_sensor_list);
        if(status != FLASH_OK){
            rt_kprintf("write_config_by_key: update sensor config failed!\n");
            return RT_FALSE;
        }
        return RT_TRUE;
    //4. write DEVICE CONFIG
    } else if(rt_strncmp(key, "CONFIG_DEVICE", rt_strlen("CONFIG_DEVICE")) == 0){
        config_device_list_t *config_device_list = rt_calloc(1, sizeof(config_device_list_t));
        if (config_device_list == RT_NULL) {
            rt_kprintf("write_config_by_key: alloc config_device_list failed!\n");
            return RT_FALSE;
        }
        config_json_to_device(json_value, config_device_list);

        status = update_config_device_list(config_device_list);
        rt_free(config_device_list);
        if(status != FLASH_OK){
            rt_kprintf("write_config_by_key: update device config failed!\n");
            return RT_FALSE;
        }
        return RT_TRUE;
    } else if(rt_strcasecmp(key, "FLASH_BLOCK_RESET") == 0){
        status = flash_block_reset();
        if(status != FLASH_OK){
            rt_kprintf("write_config_by_key: reset flash block failed!\n");
            return RT_FALSE;
        }
        return RT_TRUE;
    } else if(rt_strcasecmp(key, "TASKS") == 0){
        config_gateway_task_list_t config_gateway_task_list = {0};
        config_json_to_task_list(json_value, &config_gateway_task_list);

        status = add_or_update_config_task_list(&config_gateway_task_list);
        if(status != FLASH_OK){
            rt_kprintf("write_config_by_key: update task list failed!\n");
            return RT_FALSE;
        }
				return RT_TRUE;
    } else {
        rt_kprintf("write_config_by_key: key %s not found!\n", key);
        return RT_FALSE;
    }
};

INIT_COMPONENT_EXPORT(config_init);