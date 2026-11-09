#include "service_gateway_reboot.h"
#include "../transport/transport_message_queue.h"
#include "../transport_udp/transport_udp_service_message.h"
#include "../utils/utils.h"
#include <rtthread.h>

rt_err_t service_execute_gateway_reboot(void* transport_udp_message, void* business_data)
{
        // 实现网关重启逻辑
        service_data_reboot_t *data = (service_data_reboot_t *)business_data;
        if(data == RT_NULL)
            return RT_EINVAL;
        if(data->app_type == 1)
            rt_kprintf("Info: Rebooting main gateway...\n");
        else if(data->app_type == 2)
            rt_kprintf("Info: Rebooting DMS gateway...\n");
        else
            rt_kprintf("Error: Invalid app_type in reboot request\n");

        rt_kprintf("Gateway rebooting...\n");
         // 1. 延迟 500ms，确保日志输出完成
        rt_thread_mdelay(500);
        // 2. 调用 RT-Thread 内核重启接口（部分 BSP 已实现）
        rt_hw_cpu_reset();
        // 若 rt_hw_cpu_reset 未生效，执行硬件复位兜底
        //rt_hw_reset();
		return RT_EOK;
}

/**
* @brief 创建传感器数据上报服务实例
*
* @return service_sensor_data_report_t* 服务实例指针，失败返回NULL
*/

service_gateway_reboot_t* service_gateway_reboot_create(void)
{
	
	 // Use RT-Thread's memory allocation function instead of standard malloc
    // rt_malloc is more suitable for embedded systems with RTOS
	
   service_gateway_reboot_t* service = (service_gateway_reboot_t*)rt_malloc(sizeof(service_gateway_reboot_t));
   
   if (service == NULL) {// RT-Thread uses RT_NULL instead of NULL convention
       rt_kprintf("Error: Failed to allocate memory for service_gateway_reboot_t\n");
       return NULL;
   }
   rt_memset(service, 0, sizeof(service_gateway_reboot_t));
   service->base.service_type = SERVICE_TYPE_GATEWAY_REBOOT;
   service->base.service_execute = service_execute_gateway_reboot;
   
   return service;
}

