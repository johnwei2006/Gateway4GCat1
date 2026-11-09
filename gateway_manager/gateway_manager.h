#ifndef _GATEWAY_MANAGER_H_
#define _GATEWAY_MANAGER_H_

#include "../config/config_manager.h"
#include "../utils/utils.h"
#include "../utils/cps_list.h"
#include <stdbool.h>
#include "node_info.h"
#include "gateway_info.h"

//static gateway_info_t *gateway_info;
static cps_list_node_t *node_list;
static bool is_initialized = false;
static bool is_rtc_time_syncronized = false;
void gateway_manager_add_node(node_info_t* node); 
void gateway_manager_update_node(node_info_t* node); 
void gateway_manager_remove_node(char* p_node_id); 
void gateway_manager_get_gateway_info(gateway_info_t* gateway_info);
void gateway_manager_get_node_list(cps_list_node_t* node_list);
bool gateway_manager_rtc_time_syncronized(void);
bool gateway_manager_set_rtc_time_syncronized(bool is_rtc_time_syncronized);
static node_info_t* gateway_manager_query_node_by_id(char* p_node_id);
static rt_mutex_t gateway_manager_mutex;
static int stm32_get_mcu_id(uint32_t uid[3]);
void print_all_thread_stack(void);
static uint8_t   gateway_reboot_time[6];
void gateway_manager_set_reboot_time(uint8_t* reboot_time);
void gateway_manager_get_reboot_time(uint8_t* reboot_time);
#endif /*_GATEWAY_MANAGER_H_*/
