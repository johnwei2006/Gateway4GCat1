#include "service_ping.h"
#include "../transport/transport_message_queue.h"
#include "../utils/utils.h"
#include "../transport_lora/lora_message.h"
#include <rtthread.h>
/**
* @brief 执行ping服务
*
* 创建并发送ping消息到云平台
*/
 
rt_err_t service_execute_ping(void* param)
{
    rt_kprintf("service_execute_ping\n");
    PHYPayload_t *PHYPayload = (PHYPayload_t *)rt_malloc(sizeof(PHYPayload_t));
    if (PHYPayload == RT_NULL) {
        rt_kprintf("service_execute_ping: malloc PHYPayload error\n");
        return RT_ERROR;
    }
    rt_memset(PHYPayload, 0, sizeof(PHYPayload_t));
    PHYPayload->mac_payload = (MACPayload_t*)rt_malloc(sizeof(MACPayload_t));
    if (PHYPayload->mac_payload == NULL) {
        rt_kprintf("service_execute_ping: malloc mac_payload error\n");
        return RT_ERROR;
    }
    rt_memset(PHYPayload->mac_payload, 0, sizeof(MACPayload_t));

    //1、set event_id
    short event_id = generate_rand_short();
    PHYPayload->mac_payload->event_id[0] = event_id & 0xFF;
    PHYPayload->mac_payload->event_id[1] = (event_id >> 8) & 0xFF;

    PHYPayload->mac_payload->fcmd = FCMD_NODE_PING;
    PHYPayload->mac_payload->flen = 4;

    //3、set mac.frm_payload
    PHYPayload->mac_payload->service_data = "PONG";
    
	  rt_kprintf("service_execute_ping event_id : %d\r\n", event_id);
    //rt_kprintf("发送前: PHYPayload=%p, mac_payload=%p\n", PHYPayload, PHYPayload->mac_payload);
    put_node_downlink_message(&PHYPayload);
}

/**
* @brief 创建通知传感器数据上报批量服务实例
*
* @return service_notify_sensor_data_report_batch_t* 服务实例指针，失败返回NULL
*/
service_ping_t* service_ping_create()
{
   service_ping_t* service = (service_ping_t*)rt_malloc(sizeof(service_ping_t));
   
   if (service == NULL) {// RT-Thread uses RT_NULL instead of NULL convention
       rt_kprintf("Error: Failed to allocate memory for notify sensor data report batch service\n");
       return RT_NULL;
   }
   rt_memset(service, 0, sizeof(service_ping_t));
   service->base.service_type = FCMD_NODE_PING;
   service->base.service_execute = service_execute_ping;
   
   return service;
}


/**
* @brief 销毁网关时间同步请求服务实例
*
* @param service 服务实例指针
*/
void service_ping_destroy(service_ping_t* service)
{
   if (service != RT_NULL) {
       rt_free(service);
   }
}