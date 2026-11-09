#include "service_gateway_time_sync_response.h"
#include "../transport/transport_message_queue.h"
#include "../transport_udp/transport_udp_service_message.h"
#include "../utils/utils.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <stdio.h>
#include <time.h>
#include "../gateway_manager/rtc_manager.h"
#include "../gateway_manager/gateway_manager.h"

/**
* @brief 执行网关时间同步响应服务
*
* 接收网关时间同步响应消息
*/
 
rt_err_t service_execute_gateway_time_sync_response(void* transport_udp_message, void* business_data)
{
		if(transport_udp_message == NULL)
		{
				rt_kprintf("service_execute_gateway_time_sync_response parameter is null. \n");
        return RT_EINVAL;
		}
		transport_udp_message_t* gateway_uplink_message  	= (transport_udp_message_t*)transport_udp_message;
    service_data_time_sync_response_t *time_sync_response = (service_data_time_sync_response_t *)business_data;
    if (time_sync_response == NULL) {
        rt_kprintf("Error: time_sync_response is NULL\n");
        return RT_EINVAL;
    } 

    rt_kprintf("Time Sync Response: %02d-%02d-%02d %02d:%02d:%02d\n",
               time_sync_response->year, time_sync_response->month, time_sync_response->day,
               time_sync_response->hour, time_sync_response->minute, time_sync_response->second);

    rt_device_rtc_time_t rtc_time = {0};
    rtc_time.year  = time_sync_response->year + 1900;     
    rtc_time.month = time_sync_response->month + 1;       
    rtc_time.day   = time_sync_response->day;
    rtc_time.hour  = time_sync_response->hour;
    rtc_time.min   = time_sync_response->minute;
    rtc_time.sec   = time_sync_response->second;

    if (set_rtc_time(&rtc_time) != 0) {
        rt_kprintf("Error: set_rtc_time failed!\n");
    }

    uint8_t reboot_time[6] = {0};
    reboot_time[0] = rtc_time.year % 100;;
    reboot_time[1] = rtc_time.month;
    reboot_time[2] = rtc_time.day;
    reboot_time[3] = rtc_time.hour;
    reboot_time[4] = rtc_time.min;
    reboot_time[5] = rtc_time.sec;

    gateway_manager_set_reboot_time(reboot_time);

    rt_device_rtc_time_t new_rtc_time = {0};
    if (read_rtc_time(&new_rtc_time) == 0) {
        rt_kprintf("Now the new RTC Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                   new_rtc_time.year, new_rtc_time.month, new_rtc_time.day,
                   new_rtc_time.hour, new_rtc_time.min, new_rtc_time.sec);
    } else {
        rt_kprintf("Error: read_rtc_time failed!\n");
    }
		
		gateway_manager_set_rtc_time_syncronized(true);
		rt_kprintf("Time sync response service executed. \n");
		return RT_EOK;    
}


/**
* @brief 创建网关时间同步响应服务实例
*
* @return service_gateway_time_sync_response_t* 服务实例指针，失败返回NULL
*/
service_gateway_time_sync_response_t* service_gateway_time_sync_response_create(void)
{
	
	 // Use RT-Thread's memory allocation function instead of standard malloc
    // rt_malloc is more suitable for embedded systems with RTOS
	
   service_gateway_time_sync_response_t* service = (service_gateway_time_sync_response_t*)rt_malloc(sizeof(service_gateway_time_sync_response_t));
   
   if (service == NULL) {// RT-Thread uses RT_NULL instead of NULL convention
       rt_kprintf("Error: Failed to allocate memory for gateway time sync response service\n");
       return NULL;
   }
   rt_memset(service, 0, sizeof(service_gateway_time_sync_response_t));
   service->base.service_type    = SERVICE_TYPE_TIME_SYNC_RESPONSE;
   service->base.service_execute = service_execute_gateway_time_sync_response;
   
   return service;
}


/**
* @brief 销毁网关时间同步响应服务实例
*
* @param service 服务实例指针
*/
void service_gateway_time_sync_response_destroy(service_gateway_time_sync_response_t* service)
{
   if (service != RT_NULL) {
       rt_free(service);
   }
}