#include "gateway_manager.h"
#include <rtthread.h>
#include "../utils/cps_list.h"
#include "../config/config_manager.h"
#include "./rtc_manager.h"
#include <rtdef.h> 
#include "../flash_external/flash_block.h"
#include "../power_manager/power_manager.h"
#include "stm32f4xx_hal.h" 

int gateway_manager_init(void) {
    gateway_manager_mutex = rt_mutex_create("gateway_manager_mutex", RT_IPC_FLAG_FIFO);
    if (gateway_manager_mutex == RT_NULL) {
        rt_kprintf("Failed to create gateway_manager_mutex!\n");
        return -1;
    }	
    return RT_EOK;
}
INIT_COMPONENT_EXPORT(gateway_manager_init); 

void gateway_manager_get_gateway_info(gateway_info_t* gateway_info){
    if(gateway_info == NULL){
        return;
    }
    rt_memcpy(gateway_info->hardware_version, HEARDWARE_VERSION, FIRMWARE_VERSION_LENGTH);
    rt_memcpy(gateway_info->firmware_version, FIRMWARE_VERSION,  FIRMWARE_VERSION_LENGTH);
    
    memset(gateway_info->gateway_mcuid, 0, sizeof(gateway_info->gateway_mcuid));
    stm32_get_mcu_id(gateway_info->gateway_mcuid);
    
	  //rtc
    rt_device_rtc_time_t new_rtc_time = {0};
    if (read_rtc_time(&new_rtc_time) == 0) {
        gateway_info->rtc_time[0] = new_rtc_time.year % 100;
        gateway_info->rtc_time[1] = new_rtc_time.month;
        gateway_info->rtc_time[2] = new_rtc_time.day;
        gateway_info->rtc_time[3] = new_rtc_time.hour;
        gateway_info->rtc_time[4] = new_rtc_time.min;
        gateway_info->rtc_time[5] = new_rtc_time.sec;
    } else {
        rt_kprintf("Error: Failed to read RTC time\n");
    }

    rt_memcpy(gateway_info->latest_reboot_time, gateway_reboot_time, sizeof(gateway_reboot_time));
		//memory
    rt_uint32_t total_heap, used_heap, max_used;
    rt_memory_info(&total_heap, &used_heap, &max_used);
    gateway_info->memory_heap_total  = total_heap;
    gateway_info->memory_heap_used   = used_heap;
    gateway_info->memory_heap_free   = total_heap - used_heap;

    // 获取线程栈剩余空间（字节）
    // rt_uint32_t free_stack = rt_thread_stack_space(tid);
    // gateway_info->memory_stack_free = free_stack;
    // gateway_info->memory_stack_used = total_stack - free_stack;
    // gateway_info->memory_stack_total = total_stack;

    uint16_t used, total;
    block_print_status(&used, &total, false);
    gateway_info->flash_total           = total;
    gateway_info->flash_used            = used;
    gateway_info->uplink_message_size   = get_uplink_message_cache_size();
    gateway_info->downlink_message_size = 0;

    power_status_t power_status = {0};
    power_manager_get_status(&power_status);
    gateway_info->battery_voltage       = power_status.battery_voltage;  
    gateway_info->adapter_exists        = power_status.adapter_exists;
    gateway_info->charging_enabled      = power_status.charging_enabled;
	return;
};

void gateway_manager_get_node_list(cps_list_node_t* node_list){
    node_list = node_list;
};

void gateway_manager_add_node(node_info_t* node){
    if(!is_initialized){
        node_list = (cps_list_node_t *)cps_list_init();
        is_initialized = true;
    }
    cps_list_add(&node_list, node);
}

void gateway_manager_update_node(node_info_t* node){
    rt_mutex_take(gateway_manager_mutex, RT_WAITING_FOREVER);
    node_info_t* p_node = gateway_manager_query_node_by_id(node->node_config.node_id);
    if(p_node != NULL){
        memcpy(p_node, node, sizeof(node_info_t));
    }
    rt_mutex_release(gateway_manager_mutex);
}; 

void gateway_manager_remove_node(char* p_node_id){
    rt_mutex_take(gateway_manager_mutex, RT_WAITING_FOREVER);
    node_info_t* p_node = gateway_manager_query_node_by_id(p_node_id);
    if(p_node != NULL){
        //cps_list_remove(&node_list, p_node);
    }
    rt_mutex_release(gateway_manager_mutex);
}; 

static node_info_t* gateway_manager_query_node_by_id(char* p_node_id){
    uint8_t size = cps_list_size(node_list);
    uint8_t i;
    for(i = 1; i <= size; i++){
        node_info_t* node = (node_info_t*)cps_list_get_node(node_list, i);
        if(strcmp(node->node_config.node_id, p_node_id) == 0) {
            return node;
        }
    }
    return NULL;
};

static int stm32_get_mcu_id(uint32_t uid[3])
{
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
    rt_kprintf("STM32 MCU UID: 0x%08X 0x%08X 0x%08X\n", uid[0], uid[1], uid[2]);
}

bool gateway_manager_rtc_time_syncronized(void){
    return is_rtc_time_syncronized;
};
bool gateway_manager_set_rtc_time_syncronized(bool is_syncronized){
    is_rtc_time_syncronized = is_syncronized;
};

void gateway_manager_set_reboot_time(uint8_t* reboot_time){
     rt_memcpy(gateway_reboot_time, reboot_time, sizeof(gateway_reboot_time));
};
void gateway_manager_get_reboot_time(uint8_t* reboot_time){
    rt_memcpy(reboot_time, gateway_reboot_time, sizeof(gateway_reboot_time));
}