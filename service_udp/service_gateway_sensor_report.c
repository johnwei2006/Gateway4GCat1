#include "service_gateway_sensor_report.h"
#include <rtthread.h>
#include "../transport/transport_message_queue.h"
#include "../transport_udp/transport_udp_message.h"
#include "../transport_udp/transport_udp_service_message.h"
#include "../utils/utils.h"
#include "../utils/time_utils.h"
#include "../modbus/modbus_manager.h"
#include "../transport_udp/business_data_cache.h"
#include "../config/config_manager.h"
#include "../sensor_read/sensor_read.h"

rt_err_t service_execute_gateway_sensor_report(void* p_data, void* business_data)
{
		transport_udp_message_t* gateway_uplink_message  	= (transport_udp_message_t*)rt_malloc(sizeof(transport_udp_message_t));
		if(gateway_uplink_message == NULL)
		{
				rt_kprintf("memory allocate gateway_uplink_message failed. \n");
        return RT_ENOMEM;
		}
		rt_memset(gateway_uplink_message, 0, sizeof(transport_udp_message_t));
		
		gateway_uplink_message->transport_udp_header			= (transport_udp_header_t*)rt_malloc(sizeof(transport_udp_header_t));
	gateway_uplink_message->transport_udp_data				= (transport_udp_data_t*)rt_malloc(sizeof(transport_udp_data_t));
	gateway_uplink_message->service_data							= (service_data_t*)rt_malloc(sizeof(service_data_t));
	
	if(gateway_uplink_message->transport_udp_header == RT_NULL || 
	   gateway_uplink_message->transport_udp_data == RT_NULL || 
	   gateway_uplink_message->service_data == RT_NULL)
	{
		rt_kprintf("memory allocate sub-structures failed. \n");
		if(gateway_uplink_message->transport_udp_header) rt_free(gateway_uplink_message->transport_udp_header);
		if(gateway_uplink_message->transport_udp_data) rt_free(gateway_uplink_message->transport_udp_data);
		if(gateway_uplink_message->service_data) rt_free(gateway_uplink_message->service_data);
		rt_free(gateway_uplink_message);
		return RT_ENOMEM;
	}
	
	rt_memset(gateway_uplink_message->transport_udp_header, 0, sizeof(transport_udp_header_t));
	rt_memset(gateway_uplink_message->transport_udp_data, 	0, sizeof(transport_udp_data_t));
	rt_memset(gateway_uplink_message->service_data, 				0, sizeof(service_data_t));
        
		gateway_uplink_message->transport_udp_data->event_id    				= generate_rand_short();
		gateway_uplink_message->transport_udp_data->service_type  			= SERVICE_TYPE_GATEWAY_SENSOR_DATA_REPORT;
    gateway_uplink_message->transport_udp_data->service_version     = SERVICE_VERSION_1; 
    gateway_uplink_message->service_data->business_data_payload 		= RT_NULL;

    char* node_id = (char*)business_data;
    config_sensor_list_t *config_sensor_list = NULL;
    config_sensor_list = (config_sensor_list_t*)rt_calloc(1, sizeof(config_sensor_list_t));
    rt_strncpy(config_sensor_list->node_id, node_id, NODE_ID_LENGTH);
    flash_status status = query_config_sensor_list(config_sensor_list);

    uint8_t config_query_result = RT_EOK;
    if(status != FLASH_OK)
    {
      rt_kprintf("query_config_sensor_list failed, node_id: %s \n", node_id);
      config_query_result = 2;   //config query failed
    }

    if(config_sensor_list->sensor_count == 0)
    {
      rt_kprintf("config_sensor_list->sensor_count is 0, node_id: %s \n", node_id);
      config_query_result = 3;  //config query failed, no sensor
    }

    service_data_sensor_data_report_t* sensor_data_report = (service_data_sensor_data_report_t*)rt_calloc(1, sizeof(service_data_sensor_data_report_t));
    rt_memcpy(sensor_data_report->node_id, node_id, NODE_ID_LENGTH);
    char time_hex[7];
    rt_err_t result = get_current_time_hex(time_hex);
    if(result == RT_EOK){
      rt_memcpy(sensor_data_report->collect_time, time_hex, sizeof(sensor_data_report->collect_time));
    }
    sensor_data_report->collect_response = config_query_result;

    if(config_query_result != RT_EOK){
      sensor_data_report->sensor_data_report_items_length = 0;
      sensor_data_report->sensor_data_report_items        = RT_NULL;
    } else {
      sensor_data_report->sensor_data_report_items        = (service_data_sensor_data_report_item_t*)rt_calloc(1, sizeof(service_data_sensor_data_report_item_t) * config_sensor_list->sensor_count); 
      sensor_data_report->sensor_data_report_items_length = config_sensor_list->sensor_count;
      modbus_request_message_t *modbus_request_message    = (modbus_request_message_t*)rt_calloc(1, sizeof(modbus_request_message_t));
      modbus_response_message_t modbus_response_message   =  {0};
      rt_err_t ret = RT_EOK;
      for(int i = 0; i < config_sensor_list->sensor_count; i++)
      {
        sensor_data_report->sensor_data_report_items[i].channel     = config_sensor_list->sensors[i].channel;
        sensor_data_report->sensor_data_report_items[i].sensor_type = config_sensor_list->sensors[i].sensor_type;

        ret = modbus_request(config_sensor_list->sensors[i].read_hex, COMMAND_HEX_MAX_LENGTH, modbus_request_message, &modbus_response_message);
        if(ret != RT_EOK)
        {
          sensor_data_report->sensor_data_report_items[i].channel_result = 4;  //modbus request failed
          rt_memset(sensor_data_report->sensor_data_report_items[i].value, 0, 4);
          rt_kprintf("modbus_request failed, node_id: %s , channel: %d\n", node_id, sensor_data_report->sensor_data_report_items[i].channel);
          continue;
        }
        ret = sensor_read_by_type(sensor_data_report->sensor_data_report_items[i].sensor_type, sensor_data_report->sensor_data_report_items[i].value, modbus_response_message.value, modbus_response_message.byte_number);
        sensor_data_report->collect_response = ret;
        if(ret != RT_EOK)
        {
          sensor_data_report->sensor_data_report_items[i].channel_result = 5;  //sensor read failed
          rt_memset(sensor_data_report->sensor_data_report_items[i].value, 0, 4);
          rt_kprintf("sensor_read_by_type failed, node_id: %s , channel: %d\n", node_id, sensor_data_report->sensor_data_report_items[i].channel);
          continue;
        }
      }

      sensor_data_report->sensor_data_report_items_length = config_sensor_list->sensor_count;
      
      rt_free(modbus_request_message);
    }
    rt_free(config_sensor_list);
    put_business_data_cache(gateway_uplink_message->transport_udp_data->event_id, sensor_data_report);

    rt_kprintf("service_execute_gateway_sensor_report executed, event_id: %d \n", gateway_uplink_message->transport_udp_data->event_id);

		// 发送消息
		put_gateway_uplink_message(&gateway_uplink_message);
		return RT_EOK;
}

/**
* @brief 创建传感器数据上报服务实例
*
* @return service_sensor_data_report_t* 服务实例指针，失败返回NULL
*/

service_gateway_sensor_report_t* service_gateway_sensor_report_create(void)
{
	
	 // Use RT-Thread's memory allocation function instead of standard malloc
    // rt_malloc is more suitable for embedded systems with RTOS
	
   service_gateway_sensor_report_t* service = (service_gateway_sensor_report_t*)rt_malloc(sizeof(service_gateway_sensor_report_t));
   
   if (service == NULL) {// RT-Thread uses RT_NULL instead of NULL convention
       rt_kprintf("Error: Failed to allocate memory for service_gateway_sensor_report_t\n");
       return NULL;
   }
   rt_memset(service, 0, sizeof(service_gateway_sensor_report_t));
   service->base.service_type = SERVICE_TYPE_GATEWAY_SENSOR_DATA_REPORT;
   service->base.service_execute = service_execute_gateway_sensor_report;
   
   return service;
}