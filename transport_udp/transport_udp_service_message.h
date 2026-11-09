#ifndef TRANSPORT_UDP_SERVICE_MESSAGE_H_
#define TRANSPORT_UDP_SERVICE_MESSAGE_H_

#include <stdint.h>
#include <rtthread.h>
#include "../utils/utils.h"
#include "transport_udp_message.h"
#include "../config/config_manager.h"

typedef struct{
	  uint8_t           year;
    uint8_t           month;
    uint8_t           day;
    uint8_t           hour;
    uint8_t           minute;
    uint8_t           second;
} service_data_time_sync_response_t;

typedef struct
{
    uint8_t           channel;
    uint8_t           action;   //1: on, 2: off, 3: stop, 4: set | control result : 1 : success, 0: failed
    uint8_t           value[4];  
} service_data_gateway_device_control_item_t;  //both used by control result

typedef struct
{
    uint8_t           node_id[11];    
    uint8_t           gateway_device_control_items_length;
    service_data_gateway_device_control_item_t  *gateway_device_control_items;
} service_data_gateway_device_control_t;

typedef struct
{
    uint8_t             channel;
    uint16_t            sensor_type;
    uint8_t             channel_result;
    uint8_t             value[4];      
} service_data_sensor_data_report_item_t;

typedef struct
{
    uint8_t             node_id[11];    
    uint8_t             collect_time[6];
    uint8_t             collect_response;
    uint8_t             sensor_data_report_items_length;
    service_data_sensor_data_report_item_t  *sensor_data_report_items;
} service_data_sensor_data_report_t;


typedef struct
{
  char      upgrade_id[64];          
  uint8_t   upgrade_type;     // 升级类型: 0 - 全量升级,      1 - 差分升级
  uint8_t   app_type;         // 应用类型: 0 - bootloader,   1 - gateway
  uint32_t  firmware_size;
  //uint32_t  firmware_crc;       
  char      download_url[256];       
  char      version[32];      
  char      md5[33];           // 32 字符 + 结束符
} service_data_gateway_upgrade_t;

typedef struct
{
  char      upgrade_id[64];    
  uint8_t   app_type;         // 应用类型: 0 - bootloader,   1 - gateway
  uint8_t   status;
  uint8_t   percent;          // 升级进度百分比      
  uint8_t   error;            // 错误码
  uint8_t   node_id[11];
} service_data_gateway_upgrade_status_t;

typedef struct
{
    uint8_t           app_type;   // 1-main; 2-dms
    uint8_t           action;
} service_data_reboot_t;


typedef struct
{
    uint8_t           node_type;
    uint8_t           node_id[11];    
    uint16_t          report_interval;    
    uint8_t           lora_freq_group;
} service_data_config_node_item_t;

typedef struct
{
    uint8_t            node_size;   
    service_data_config_node_item_t  *node_config_items;
} service_data_config_node_t;

typedef struct
{
    uint8_t            result;   
    uint8_t            message[MAX_RESPONSE_MESSAGE_LENGTH];
} service_data_config_response_t;

typedef struct
{
    uint8_t           channel;
    uint8_t           result;   //1: on, 2: off, 3: stop, 4: set
} service_data_gateway_device_control_response_item_t;

typedef struct
{
    uint8_t           node_id[11];       
    uint8_t           device_size;   
    service_data_gateway_device_control_response_item_t  *device_control_response_items;
} service_data_gateway_device_control_response_t;
#endif /*TRANSPORT_UDP_SERVICE_MESSAGE_H_*/