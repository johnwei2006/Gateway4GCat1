#ifndef TRANSPORT_UDP_SERVICE_MESSAGE_H_
#define TRANSPORT_UDP_SERVICE_MESSAGE_H_

#include <stdint.h>
#include <rtthread.h>
#include "../utils/utils.h"
#include "lora_message.h"
#include "../config/config_manager.h"

typedef struct {
    uint8_t             firm_type;
    uint8_t             firm_version[4];
    uint8_t             uboot_version[4];
    uint8_t             mcu_id[10];
    uint8_t             freq_group;
} node_join_request_data_t;

typedef struct {
   uint8_t  		   result;
   uint8_t 		      token;
} node_join_response_data_t;

typedef struct
{
   uint8_t  	*message_id;
   uint8_t  	message_size;
} message_acknowledge;

typedef struct NodeStatus
{
   uint8_t  		battery;
   uint8_t  		rssi;
   uint8_t        snr;
} NodeStatus_t;


typedef struct{
	  uint8_t           year;
    uint8_t           month;
    uint8_t           day;
    uint8_t           hour;
    uint8_t           minute;
    uint8_t           second;
} service_message_time_sync_response_t;

typedef struct {
  uint8_t       default_node_id[NODE_ID_LENGTH];
  uint8_t       gateway_id[GATEWAY_ID_LENGTH];
  uint8_t       report_interval;
  uint8_t       firmware_version[5];
  uint8_t       hardware_version[5];
} service_message_node_info_t;

typedef struct
{
    uint8_t           channel;
    uint8_t           action;   //1: on, 2: off, 3: stop, 4: set
    uint8_t           value[4];  
} service_data_device_control_item_t;

typedef struct
{
    uint8_t           node_id[11];    
    service_data_device_control_item_t  *device_control_items;
    uint8_t           device_control_items_length;
} service_data_device_control_t;

typedef struct
{
    uint8_t           channel;
    uint8_t           value[4];      
} service_data_sensor_data_report_item_t;

typedef struct
{
    uint8_t         collect_time[6];
    service_data_sensor_data_report_item_t  *sensor_data_report_items;
    uint8_t         sensor_data_report_items_length;
} service_data_sensor_data_report_t;


#endif /*TRANSPORT_UDP_SERVICE_MESSAGE_H_*/
