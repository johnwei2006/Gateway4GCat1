#include "service_node_data_report.h"
#include "../transport/transport_message_queue.h"
#include "../transport_udp/transport_udp_service_message.h"
#include "../utils/utils.h"
#include "../utils/time_utils.h"
#include <rtthread.h>

rt_err_t service_execute_node_data_report(void* param)
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
		
    rt_memset(gateway_uplink_message->transport_udp_header, 0, sizeof(transport_udp_header_t));
    rt_memset(gateway_uplink_message->transport_udp_data, 	0, sizeof(transport_udp_data_t));
    rt_memset(gateway_uplink_message->service_data, 				0, sizeof(service_data_t));
        
		gateway_uplink_message->transport_udp_data->event_id    				= generate_rand_short();
		gateway_uplink_message->transport_udp_data->service_type  			= SERVICE_TYPE_GATEWAY_SENSOR_DATA_REPORT;
    gateway_uplink_message->transport_udp_data->service_version     = SERVICE_VERSION_1; 
    gateway_uplink_message->service_data->business_data_payload 		= RT_NULL;
  
    service_data_sensor_data_report_t* sensor_data_report = (service_data_sensor_data_report_t*)rt_malloc(sizeof(service_data_sensor_data_report_t));
    service_data_sensor_data_report_item_t* sensor_data_report_items = (service_data_sensor_data_report_item_t*)rt_malloc(sizeof(service_data_sensor_data_report_item_t) * 2); 
    sensor_data_report_items[0].channel = 1;
    sensor_data_report_items[0].value[0] = 0x00;
    sensor_data_report_items[0].value[1] = 0x00;
    sensor_data_report_items[0].value[2] = 0x27;
    sensor_data_report_items[0].value[3] = 0x10; // 10000

    sensor_data_report_items[1].channel = 2;
    sensor_data_report_items[1].value[0] = 0x00;
    sensor_data_report_items[1].value[1] = 0x00;
    sensor_data_report_items[1].value[2] = 0x03;
    sensor_data_report_items[1].value[3] = 0xE8;

    sensor_data_report->sensor_data_report_items = sensor_data_report_items;
    sensor_data_report->sensor_data_report_items_length = 2;
    char time_hex[7];
    rt_err_t result = get_current_time_hex(time_hex);
    if(result == RT_EOK){
      rt_strncpy(sensor_data_report->collect_time, time_hex, 6);
    }
     
    gateway_uplink_message->service_data->business_data = sensor_data_report;

    rt_kprintf("service_execute_sensor_data_report executed, event_id: %d \n", gateway_uplink_message->transport_udp_data->event_id);
		// 发送消息
		put_gateway_uplink_message(&gateway_uplink_message);

    /*
    rt_free(gateway_uplink_message->transport_udp_header);
    rt_free(gateway_uplink_message->transport_udp_data);
    rt_free(gateway_uplink_message->service_data);
    rt_free(gateway_uplink_message);
    */
		return RT_EOK;
}

/**
* @brief 创建传感器数据上报服务实例
*
* @return service_sensor_data_report_t* 服务实例指针，失败返回NULL
*/

service_node_data_report_t* service_node_data_report_create(void)
{
	
	 // Use RT-Thread's memory allocation function instead of standard malloc
    // rt_malloc is more suitable for embedded systems with RTOS
	
   service_node_data_report_t* service = (service_node_data_report_t*)rt_malloc(sizeof(service_node_data_report_t));
   
   if (service == RT_NULL) {// RT-Thread uses RT_NULL instead of NULL convention
       rt_kprintf("Error: Failed to allocate memory for service_sensor_data_report_t\n");
       return NULL;
   }
   rt_memset(service, 0, sizeof(service_node_data_report_t));
   service->base.service_type = FCMD_NODE_SENSOR_DATA_UPLOAD;
   service->base.service_execute = service_execute_node_data_report;
   
   return service;
}