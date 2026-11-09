#include "service_gateway_info_query.h"
#include "../transport/transport_message_queue.h"
#include "../config/config_manager.h"
#include "../transport_udp/transport_udp_service_message.h"
#include "../transport_udp/transport_udp_message.h"
#include "../utils/utils.h"
#include <rtthread.h>
#include <stdio.h>
#include <string.h>
#include "../lte/lte_app.h"
#include "../gateway_manager/gateway_manager.h"

/**
* @brief 执行网关信息上报服务
*
* 创建并发送网关信息上报消息到云平台
*/
 
rt_err_t service_execute_gateway_info_query(void* transport_udp_message, void* business_data){
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
        
		gateway_uplink_message->transport_udp_data->event_id    				= ((transport_udp_message_t*)transport_udp_message)->transport_udp_data->event_id;
		gateway_uplink_message->transport_udp_data->service_type  			= SERVICE_TYPE_GATEWAY_INFO_REPORT;
    gateway_uplink_message->transport_udp_data->service_version     = SERVICE_VERSION_1; 

    gateway_info_t *gateway_info = (gateway_info_t*)rt_malloc(sizeof(gateway_info_t));
    if (gateway_info == NULL) {
        rt_kprintf("Error: Failed to allocate gateway_info\n");
        free_transport_udp_message(gateway_uplink_message);
        return -RT_ENOMEM;
    }
    memset(gateway_info, 0, sizeof(gateway_info_t));
    
    gateway_manager_get_gateway_info(gateway_info);

    gateway_info->lte_info = (lte_info_t*)rt_malloc(sizeof(lte_info_t));
    if (gateway_info->lte_info == NULL) {
        rt_kprintf("Error: Failed to allocate lte_info\n");
        rt_free(gateway_info->lte_info);
        gateway_info->lte_info = RT_NULL;
        rt_free(gateway_info);
        gateway_info = RT_NULL;

        free_transport_udp_message(gateway_uplink_message);
        return -RT_ENOMEM;
    }

    get_lte_info(gateway_info->lte_info);
  
    gateway_info->gateway_config = (config_gateway_serial_t*)rt_malloc(sizeof(config_gateway_serial_t));
    if (gateway_info->gateway_config == NULL) {
        rt_kprintf("Error: Failed to allocate gateway_config\n");
        rt_free(gateway_info->lte_info);
        gateway_info->lte_info = RT_NULL;
        rt_free(gateway_info);
        gateway_info = RT_NULL;

        free_transport_udp_message(gateway_uplink_message);
        return -RT_ENOMEM;
    }

    read_config_gateway_serial(gateway_info->gateway_config);
    //gateway_uplink_message->service_data->business_data = &gateway_info;
        
    rt_kprintf("Gateway info query service executed. \n");
		// 发送消息
    put_business_data_cache(gateway_uplink_message->transport_udp_data->event_id, gateway_info);
	  rt_err_t result = put_gateway_uplink_message(&gateway_uplink_message);
    if(result != RT_EOK){
        rt_kprintf("Warning: put_gateway_uplink_message failed, free memory\n");
        free_transport_udp_message(gateway_uplink_message);

        rt_free(gateway_info->lte_info);
        rt_free(gateway_info->gateway_config);
        rt_free(gateway_info);
        gateway_info = RT_NULL;

        return result;
    }
    gateway_uplink_message = RT_NULL;
    
    return RT_EOK;
}

/**
* @brief 创建网关信息上报服务实例
*
* @return service_gateway_info_report_t* 服务实例指针，失败返回NULL
*/
service_gateway_info_query_t* service_gateway_info_query_create()
{
	
	 // Use RT-Thread's memory allocation function instead of standard malloc
    // rt_malloc is more suitable for embedded systems with RTOS
	
   service_gateway_info_query_t* service = (service_gateway_info_query_t*)rt_malloc(sizeof(service_gateway_info_query_t));
   
   if (service == RT_NULL) {
       rt_kprintf("Error: Failed to allocate memory for gateway info query service\n");
       return RT_NULL;
   }
   rt_memset(service, 0, sizeof(service_gateway_info_query_t));
   service->base.service_type = SERVICE_TYPE_GATEWAY_INFO_QUERY;
   service->base.service_execute = service_execute_gateway_info_query;
   
   return service;
}


/**
* @brief 销毁网关信息上报服务实例
*
* @param service 服务实例指针
*/
void service_gateway_info_query_destroy(service_gateway_info_query_t* service)
{
   if (service != RT_NULL) {
       rt_free(service);
   }
}