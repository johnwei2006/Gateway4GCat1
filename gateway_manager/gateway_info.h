#ifndef __GATEWAY_H__
#define __GATEWAY_H__

#include "../config/config_manager.h"
#include "../utils/utils.h"
#include "../lte/lte_app.h"
#include <time.h>

typedef struct
{
    uint8_t                 hardware_version[FIRMWARE_VERSION_LENGTH];    
    uint8_t                 firmware_version[FIRMWARE_VERSION_LENGTH];
    uint32_t                gateway_mcuid[3];
    config_gateway_serial_t *gateway_config;
    lte_info_t              *lte_info;
    uint8_t                 rtc_time[6];
    uint8_t                 latest_reboot_time[6];
    uint16_t                memory_heap_total;
    uint16_t                memory_heap_used;
    uint16_t                memory_heap_free;
    uint16_t                memory_stack_total;
    uint16_t                memory_stack_used;
    uint16_t                memory_stack_free;
    uint16_t                flash_total;
    uint16_t                flash_used;
    uint16_t                uplink_message_size;    
    uint16_t                downlink_message_size;    
    float                   battery_voltage;
    rt_bool_t               adapter_exists;       // Adapter status (RT_TRUE=exists, RT_FALSE=none)
    rt_bool_t               charging_enabled; 
    double                  lat;
    double                  lnt;
} gateway_info_t;

#endif /*__GATEWAY_H__*/
