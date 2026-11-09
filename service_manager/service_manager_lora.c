#include "service_manager_lora.h"
#include <rtthread.h>
#include "../service_lora/service_ping.h"
#include "../service_lora/service_node_join_request.h"
#include "../service_lora/service_notify_sensor_data_report_batch.h"

#define SECOND_TO_MS(sec)  ((sec) * 1000)    
#define MINUTE_TO_SEC(min) ((min) * 60)      
#define MINUTE_TO_MS(min)  (MINUTE_TO_SEC(min) * 1000)  
#define THREAD_PRIORITY         10
#define THREAD_TIMESLICE       30
#define MINUTES                 30

static void service_manager_period_execute(void *parameter)
{
    rt_int32_t report_interval = MINUTE_TO_MS(5); 
    service_t* service;
    while(1) {
				service = query_lora_service_by_type(FCMD_NODE_SENSOR_DATA_UPLOAD_NOTICE_BATCH);
        if (service != RT_NULL) {  
            service->service_execute(RT_NULL, RT_NULL);
            rt_kprintf("Service notify sensor data upload executed\n");
        } else {
            rt_kprintf("Service notify sensor data upload service not found!\n");
        }
        rt_thread_mdelay(report_interval);
    }
}

int service_manager_lora_init(void){
 	  //1. create service list
    //	1.1 service_ping
		service_ping_t* service_ping = service_ping_create();
    if (service_ping == NULL) {
				rt_kprintf("Failed to create service_ping!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_ping);        
	  //	1.2 service_notify_sensor_data_report_batch
		service_notify_sensor_data_report_batch_t* service_notify_sensor_data_report_batch = service_notify_sensor_data_report_batch_create();
    if (service_notify_sensor_data_report_batch == NULL) {
				rt_kprintf("Failed to create service_notify_sensor_data_report_batch!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_notify_sensor_data_report_batch);	

    //	1.3 service_node_join_request
		service_node_join_request_t* service_node_join_request = service_node_join_request_create();
    if (service_node_join_request == NULL) {
				rt_kprintf("Failed to create service_node_join_request!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_node_join_request);	

    //2.create and startup period_report_service_thread
    if(period_execute_service_lora_thread == RT_NULL){
        period_execute_service_lora_thread = rt_thread_create("period_execute_service_lora_thread", service_manager_period_execute, RT_NULL, 512, 10, 30);
        if (period_execute_service_lora_thread != RT_NULL)
        {
            rt_err_t ret = rt_thread_startup(period_execute_service_lora_thread);							
						if (ret != RT_EOK) {
              rt_kprintf("failed to period_execute_service_lora_thread %d\n", ret);  
              rt_thread_delete(period_execute_service_lora_thread);
              period_execute_service_lora_thread = RT_NULL;
						} else {
              rt_kprintf("success to startup period_execute_service_lora_thread\r\n");
							is_service_lora_initialized = true;
						}
        }
    }                                                                                                                		
		return RT_EOK;
}


rt_err_t service_manager_lora_process_uplink(PHYPayload_t *PHYPayload)
{
    if(!is_service_lora_initialized){
        service_manager_lora_init();
    }

    service_t* service = query_lora_service_by_type(PHYPayload->mac_payload->fcmd);
    if(!service){
        return RT_EINVAL;
    }
    return service->service_execute(PHYPayload, RT_NULL);
}

static service_t* query_lora_service_by_type(const uint8_t type) {  
    if (service_head_node == NULL) { 
        return NULL;
    }   
    uint8_t size = cps_list_size(service_head_node);    
    if (size == 0) {  
        return NULL;
    }    
    for (uint8_t i = 1; i <= size; i++) {
        const service_t* service = (const service_t*)cps_list_get_node(service_head_node, i);
        if (service == NULL) {  
            continue;          // Skip invalid nodes
        }
        if (service->service_type == type) {
            return (service_t*)service;   
        }
    }
    return NULL;
}
void service_manager_lora_finalize(){
    cps_list_free(service_head_node);
}

//INIT_APP_EXPORT(service_manager_lora_init);