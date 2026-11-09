#ifndef _NODE_H_
#define _NODE_H_

#include "../config/config_manager.h"
#include "../utils/utils.h"
#include <time.h>

typedef enum {
    NODE_STATUS_OFFLINE = 0,
    NODE_STATUS_ONLINE  = 1,
    NODE_STATUS_ERROR   = 2
} node_status_t;

typedef struct
{
    config_node_t       node_config;
    node_status_t       node_status;
    uint8_t             hardware_version[FIRMWARE_VERSION_LENGTH];    
    uint8_t             firmware_version[FIRMWARE_VERSION_LENGTH];
    uint8_t             node_mcuid[3];
    uint16_t            node_memory_total;
    uint16_t            node_memory_used;
    uint16_t            node_flash_total;
    uint16_t            node_flash_used;
    uint16_t            node_cached_record_size;    
    float               battery;
    int                 rssi;
    int                 snr;
    uint8_t             node_token;
    //uint8_t           node_latest_connect_time[6];    
    time_t              node_latest_connect_time;
    time_t              node_latest_join_time;    
} node_info_t;

#endif /*_NODE_H_*/
