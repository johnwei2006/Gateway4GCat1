#include "service_notify_sensor_data_report_batch.h"
#include "../transport/transport_message_queue.h"
#include "../utils/utils.h"
#include "../transport_lora/lora_message.h"
#include <rtthread.h>
/**
* @brief 执行通知传感器数据上报批量通知服务
*
* 创建并发送通知传感器数据上报批量消息到云平台
*/
 
rt_err_t service_execute_notify_sensor_data_report_batch(void* param)
{
    rt_kprintf("service_execute_notify_sensor_data_report_batch\n");
    PHYPayload_t *PHYPayload = (PHYPayload_t *)rt_malloc(sizeof(PHYPayload_t));
    if (PHYPayload == RT_NULL) {
        rt_kprintf("service_execute_notify_sensor_data_report_batch: malloc PHYPayload error\n");
        return RT_ERROR;
    }
    rt_memset(PHYPayload, 0, sizeof(PHYPayload_t));
    PHYPayload->mac_payload = (MACPayload_t*)rt_malloc(sizeof(MACPayload_t));
    if (PHYPayload->mac_payload == NULL) {
        rt_kprintf("service_execute_notify_sensor_data_report_batch: malloc mac_payload error\n");
        return RT_ERROR;
    }
    rt_memset(PHYPayload->mac_payload, 0, sizeof(MACPayload_t));

    //1、set event_id
    short event_id = generate_rand_short();
    PHYPayload->mac_payload->event_id[0] = event_id & 0xFF;
    PHYPayload->mac_payload->event_id[1] = (event_id >> 8) & 0xFF;

    PHYPayload->mac_payload->fcmd = FCMD_NODE_SENSOR_DATA_UPLOAD_NOTICE_BATCH;
    PHYPayload->mac_payload->flen = 0;

    //3、set mac.frm_payload
    PHYPayload->mac_payload->service_data = RT_NULL;
    
		rt_kprintf("service_execute_notify_sensor_data_report_batch event_id : %d\r\n", event_id);
    //rt_kprintf("发送前: PHYPayload=%p, mac_payload=%p\n", PHYPayload, PHYPayload->mac_payload);
    put_node_downlink_message(&PHYPayload);
}

/**
* @brief 创建通知传感器数据上报批量服务实例
*
* @return service_notify_sensor_data_report_batch_t* 服务实例指针，失败返回NULL
*/
service_notify_sensor_data_report_batch_t* service_notify_sensor_data_report_batch_create()
{
   service_notify_sensor_data_report_batch_t* service = (service_notify_sensor_data_report_batch_t*)rt_malloc(sizeof(service_notify_sensor_data_report_batch_t));
   
   if (service == NULL) {// RT-Thread uses RT_NULL instead of NULL convention
       rt_kprintf("Error: Failed to allocate memory for notify sensor data report batch service\n");
       return RT_NULL;
   }
   rt_memset(service, 0, sizeof(service_notify_sensor_data_report_batch_t));
   service->base.service_type = FCMD_NODE_SENSOR_DATA_UPLOAD_NOTICE_BATCH;
   service->base.service_execute = service_execute_notify_sensor_data_report_batch;
   
   return service;
}


/**
* @brief 销毁网关时间同步请求服务实例
*
* @param service 服务实例指针
*/
void service_notify_sensor_data_report_batch_destroy(service_notify_sensor_data_report_batch_t* service)
{
   if (service != RT_NULL) {
       rt_free(service);
   }
}