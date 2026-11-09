#include "service_gateway_time_sync_request.h"
#include "../transport/transport_message_queue.h"
#include "../utils/utils.h"
#include <rtthread.h>
/**
* @brief 执行网关时间同步请求服务
*
* 创建并发送网关时间同步请求消息到云平台
*/
 
rt_err_t service_execute_gateway_time_sync_request(void* p_data, void* business_data)
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
		gateway_uplink_message->transport_udp_data->service_type  			= SERVICE_TYPE_TIME_SYNC_REQUEST;
    gateway_uplink_message->transport_udp_data->service_version     = SERVICE_VERSION_1; 
    gateway_uplink_message->service_data->business_data_payload 		= RT_NULL;
    gateway_uplink_message->service_data->business_data_payload_length= 0;
    gateway_uplink_message->service_data->business_data 						= RT_NULL;
        
    rt_kprintf("Time sync request service executed. \n");
		// 发送消息
		put_gateway_uplink_message(&gateway_uplink_message);
    return RT_EOK;
}

/**
* @brief 创建网关心跳服务实例
*
* @return service_gateway_heartbeat_t* 服务实例指针，失败返回NULL
*/
service_gateway_time_sync_request_t* service_gateway_time_sync_request_create()
{
	
	 // Use RT-Thread's memory allocation function instead of standard malloc
    // rt_malloc is more suitable for embedded systems with RTOS
	
   service_gateway_time_sync_request_t* service = (service_gateway_time_sync_request_t*)rt_malloc(sizeof(service_gateway_time_sync_request_t));
   
   if (service == NULL) {// RT-Thread uses RT_NULL instead of NULL convention
       rt_kprintf("Error: Failed to allocate memory for gateway time sync request service\n");
       return RT_NULL;
   }
   rt_memset(service, 0, sizeof(service_gateway_time_sync_request_t));
   service->base.service_type = SERVICE_TYPE_TIME_SYNC_REQUEST;
   service->base.service_execute = service_execute_gateway_time_sync_request;
   
   return service;
}


/**
* @brief 销毁网关时间同步请求服务实例
*
* @param service 服务实例指针
*/
void service_gateway_time_sync_request_destroy(service_gateway_time_sync_request_t* service)
{
   if (service != NULL) {
       free(service);
   }
}