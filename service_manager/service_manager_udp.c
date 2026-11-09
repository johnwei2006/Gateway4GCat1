#include "service_manager_udp.h"
#include <rtthread.h>
#include "../service_udp/service_gateway_heartbeat.h"
#include "../service_udp/service_gateway_config.h"
#include "../service_udp/service_gateway_config_query.h"
#include "../service_udp/service_gateway_config_node.h"
#include "../service_udp/service_gateway_config_node_query.h"
#include "../service_udp/service_gateway_config_sensor.h"
#include "../service_udp/service_gateway_config_sensor_query.h"
#include "../service_udp/service_gateway_config_device.h"
#include "../service_udp/service_gateway_config_device_query.h"
#include "../service_udp/service_gateway_time_sync_request.h"
#include "../service_udp/service_gateway_time_sync_response.h"
#include "../service_udp/service_gateway_reboot.h"
#include "../service_udp/service_gateway_info_query.h"
#include "../service_udp/service_gateway_sensor_report.h"
#include "../service_udp/service_gateway_device_control.h"
#include "../service_udp/service_gateway_upgrade.h"
#include "../service_udp/service_gateway_config_task.h"
#include "../service_udp/service_gateway_config_task_query.h"
#include "../service_udp/service_gateway_config_task_list.h"
#include "../config/config_manager.h"
#include <rthw.h>
#include "../gateway_manager/rtc_manager.h"

#define SECOND_TO_MS(sec)  ((sec) * 1000)    
#define MINUTE_TO_SEC(min) ((min) * 60)      
#define MINUTE_TO_MS(min)  (MINUTE_TO_SEC(min) * 1000)  
#define THREAD_PRIORITY             10
//#define THREAD_STACK_SIZE         1024
#define THREAD_TIMESLICE            30
#define MINUTES                     30
#define EC200_LOST_CONNECTION_RETRY_TIMES 5
static uint8_t  udp_unconnected_count = 0;

bool is_service_udp_initialized = false;
struct cps_list_node *service_head_node = NULL;
rt_thread_t heartbeat_thread     = RT_NULL;
rt_thread_t period_report_thread = RT_NULL;

void service_manager_heartbeat(void *parameter)
{
    rt_int32_t heartbeat_interval = SECOND_TO_MS(1 * 60);
    config_gateway_serial_t gateway_config = {0};
    flash_status status = read_config_gateway_serial(&gateway_config);
    if(status != FLASH_OK){
      rt_kprintf("Info: read_config_gateway_serial failed\n");
      heartbeat_interval = MINUTE_TO_MS(1); // use default 2 minutes
    } else {
      if(gateway_config.heartbeat_interval > 5 * 60 || gateway_config.heartbeat_interval < 10) // if configured heartbeat interval is greater than 10 minutes, use default 2 minutes
          heartbeat_interval = MINUTE_TO_MS(2);
      else
          heartbeat_interval = SECOND_TO_MS(gateway_config.heartbeat_interval); 
    } 

    service_t* service;
    while(1) {
        // 1. local task execution（ignore connection status）
        /*
        config_gateway_task_list_t config_task_list = {0};
        status = query_config_task_list(&config_task_list);
        if(status != FLASH_OK){
          rt_kprintf("Info: query_config_task_list failed\n");
        } else {
          bool trigger_matched = 0;
          rt_kprintf("Info: config_task_list has %d tasks\n", config_task_list.task_count);
          for(uint8_t i = 0; i < config_task_list.task_count; i++){
            config_gateway_task_t* task = &config_task_list.config_tasks[i];
            rt_kprintf("Info: task_id: %d, enabled: %d, trigger_type: %d\n", task->task_id, task->enabled, task->trigger_type);
            rt_kprintf("Info: task_id: %d, has %d actions\n", task->task_id, task->action_size);
            for(uint8_t j = 0; j < task->action_size; j++){
                config_gateway_task_action_t* action = &task->actions[j];
                rt_kprintf("Info: action %d has node_id %s, channel %d, action %d, value %d\n", j, action->node_id, action->channel, action->action, action->value);
            }
            if(task->trigger_type == TRIGGER_TYPE_INTERVAL){
                rt_kprintf("Info: task_id: %d, be triggered at time %d:%d:%d every day\n", task->task_id, task->trigger_time[0], task->trigger_time[1], task->trigger_time[2]);
                if(task->enabled == 0){
                    continue;
                }
                rt_device_rtc_time_t new_rtc_time = {0};
                if (read_rtc_time(&new_rtc_time) == 0) {
                  if(new_rtc_time.hour == task->trigger_time[0] && (new_rtc_time.min - task->trigger_time[1]) < heartbeat_interval){
                    trigger_matched = 1;
                  }
                } else {
                    rt_kprintf("Error: Failed to read RTC time\n");
                }                
            } else {
                rt_kprintf("Info: task_id: %d, has %d triggers\n", task->task_id, task->trigger_size);
                bool last_trigger_matched, current_trigger_matched = 0;
                for(uint8_t j = 0; j < task->trigger_size; j++){
                    config_gateway_task_trigger_t* trigger = &task->triggers[j];
                    rt_kprintf("Info: trigger %d has node_id %s, channel %d, compare_type %d, value %d\n", j, trigger->node_id, trigger->channel, trigger->compare_type, trigger->value);
                    config_sensor_t config_sensor = {0};
                    status = query_config_sensor_by_channel(&config_sensor, trigger->node_id, trigger->channel);
                    if(status != FLASH_OK){
                        rt_kprintf("Info: query_config_sensor_by_channel failed\n");
                        continue;
                    }
                    if(task->triggers[j].join_type == TRIGGER_JOIN_TYPE_AND){
                        // 1. read sensor value using config_sensor.read_hex
                        // 2. compare sensor value with task->triggers[j].value
                        if(!current_trigger_matched)
                          break;
                        last_trigger_matched = current_trigger_matched;
                        if(j == task->trigger_size - 1){
                          trigger_matched = 1;
                          break;
                        }
                    } else {                        
                        // 1. read sensor value using config_sensor.read_hex
                        // 2. compare sensor value with task->triggers[j].value
                        if(current_trigger_matched){
                          trigger_matched = 1;
                          break;
                        }
                    }
                }
                if(task->enabled == 0){
                    continue;
                }
            }
            if(trigger_matched){
              rt_kprintf("Info: task_id: %d, triggered\n", task->task_id);
              config_device_t config_device = {0};
              for(uint8_t k = 0; k < task->action_size; k++){
                status = query_config_device_by_channel(&config_device, task->actions[k].node_id, task->actions[k].channel);
                if(status != FLASH_OK){
                  rt_kprintf("Info: query_config_device_by_channel failed\n");
                  continue;
                }
                if(task->actions[k].action == ACTION_TYPE_ON){
                  //config_device.on_hex = 1;
                  rt_kprintf("Info: node_id %s, channel %d, turn device on\n", task->actions[k].node_id, task->actions[k].channel);
                } else if(task->actions[k].action == ACTION_TYPE_OFF){
                  //config_device.on_hex = 0;
                  rt_kprintf("Info: node_id %s, channel %d, turn device off\n", task->actions[k].node_id, task->actions[k].channel);
                } else if(task->actions[k].action == ACTION_TYPE_SET){
                  //(config_device.set_hex, task->actions[k].value)
                  rt_kprintf("Info: node_id %s, channel %d, set device value to %d\n", task->actions[k].node_id, task->actions[k].channel, task->actions[k].value);
                }
              }              
            } else {
              rt_kprintf("Info: task_id: %d, not triggered\n", task->task_id);
            }
          }
        }*/

        // 2. heartbeat service execution
        rt_thread_mdelay(heartbeat_interval);

        lte_info_t lte_info = {0};
        get_lte_info(&lte_info);
        if(lte_info.lte_state != UDP_STATE_CONNECTED){
          udp_unconnected_count++;
          if(udp_unconnected_count > EC200_LOST_CONNECTION_RETRY_TIMES){
            //ec200_power_pin_init();
            rt_kprintf("Info: lte connection lost too long, try to reboot\n");
            rt_hw_cpu_reset();
          }
          continue;
        } else {
           udp_unconnected_count = 0;
        }


        service = query_service_by_type(SERVICE_TYPE_HEARTBEAT);
        if (service != RT_NULL) {  
            service->service_execute(RT_NULL, RT_NULL);
            rt_kprintf("Service heartbeat executed\n");
        } else {
            rt_kprintf("Service heartbeat service not found!\n");
        }

        // 3. history data report
        //   1) read history data from flash 
        //   2) report history data to platform then delete it from flash
		}
}

void service_manager_period_execute(void *parameter)
{
    config_gateway_serial_t gateway_config = {0};
    flash_status status = read_config_gateway_serial(&gateway_config);
    if(status != FLASH_OK){
        rt_kprintf("Info: read_config_gateway_serial failed\n");
        return;
    }

    rt_int32_t report_interval = SECOND_TO_MS(gateway_config.report_interval); 
    service_t* service;
    while(1) {
        if(report_interval == 0)
          rt_thread_mdelay(60 * 1000);
        else 
          rt_thread_mdelay(report_interval);
        
        // 1. sensor data report
        config_node_list_t config_node_list = {0};
        flash_status status = query_config_node_list(&config_node_list);
        if(status != FLASH_OK){
          rt_kprintf("Info: query_config_node_list failed\n");
          continue;
        }

        if(config_node_list.nodes_count == 0){
          rt_kprintf("Info: config_node_list is empty, no sensor need to be reported\n");
          continue;
        }

        for(uint8_t i = 0; i < config_node_list.nodes_count; i++){
          config_node_serial_t* node = &config_node_list.config_nodes[i];
          service = query_service_by_type(SERVICE_TYPE_GATEWAY_SENSOR_DATA_REPORT);
          if (service != RT_NULL) {  
              //service->service_execute(RT_NULL,node->node_id);
              rt_kprintf("Service sensor report executed\n");
          } else {
              rt_kprintf("Service sensor report service not found!\n");
          }    
        }
    }
}
int service_manager_udp_init(void){
 		//1. create service list
		//	1.1 service_gateway_heartbeat
		service_gateway_heartbeat_t* service_gateway_heartbeat = service_gateway_heartbeat_create();
    if (service_gateway_heartbeat == NULL) {
				rt_kprintf("Failed to create service_sys_config!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_heartbeat);	

		//	2.2 service_time_sync_request   
		service_gateway_time_sync_request_t* service_gateway_time_sync_request = service_gateway_time_sync_request_create();
    if (service_gateway_time_sync_request == NULL) {
				rt_kprintf("Failed to create service_gateway_time_sync_request!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_time_sync_request);	

		//	2.3 service_gateway_time_sync_response   
		service_gateway_time_sync_response_t* service_gateway_time_sync_response = service_gateway_time_sync_response_create();
    if (service_gateway_time_sync_response == NULL) {
				rt_kprintf("Failed to create service_gateway_time_sync_response!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_time_sync_response);	
      
		//	2.4 service_gateway_info_query   
    service_gateway_info_query_t* service_gateway_info_query = service_gateway_info_query_create();   
		if (service_gateway_info_query == NULL) {
				rt_kprintf("Failed to create service_gateway_info_query!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_info_query);	
    
    //	2.5 service_gateway_config
		service_gateway_config_t* service_gateway_config = service_gateway_config_create();
		if (service_gateway_config == NULL) {
				 rt_kprintf("Failed to create service_gateway_config!\n");
				 return -RT_ENOMEM;
		}			
	  cps_list_add(&service_head_node, service_gateway_config);		
		
	  //	2.6 service_gateway_config_query
    service_gateway_config_query_t* service_gateway_config_query = service_gateway_config_query_create();
		if (service_gateway_config_query == NULL) {
				rt_kprintf("Failed to create service_gateway_config_query!\n");
        return -RT_ENOMEM;
    }
		cps_list_add(&service_head_node, service_gateway_config_query);	

    //	2.7 service_device_control   
    service_gateway_device_control_t* service_gateway_device_control = service_gateway_device_control_create();   
		if (service_gateway_device_control == NULL) {
				rt_kprintf("Failed to create service_gateway_device_control!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_device_control);	 

    //	2.8 service_sensor_data_report   
    service_gateway_sensor_report_t* service_gateway_sensor_report = service_gateway_sensor_report_create();   
		if (service_gateway_sensor_report == NULL) {
				rt_kprintf("Failed to create service_gateway_sensor_report!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_sensor_report);	 

    //	2.9 service_gateway_reboot   
    service_gateway_reboot_t* service_gateway_reboot = service_gateway_reboot_create();   
		if (service_gateway_reboot == NULL) {
				rt_kprintf("Failed to create service_gateway_reboot!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_reboot);	 

    //  2.10 NODE_CONFIG
    service_gateway_config_node_t* service_gateway_config_node = service_gateway_config_node_create();
		if (service_gateway_config_node == NULL) {
				rt_kprintf("Failed to create service_gateway_node_config!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_config_node);	 

    //  2.10 NODE_CONFIG query
    service_gateway_config_node_query_t* service_gateway_config_node_query = service_gateway_config_node_query_create();
		if (service_gateway_config_node_query == NULL) {
				rt_kprintf("Failed to create service_gateway_node_config_query!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_config_node_query);	 

    //  2.11 SENSOR_CONFIG
    service_gateway_config_sensor_t* service_gateway_sensor_config = service_gateway_config_sensor_create();
		if (service_gateway_sensor_config == NULL) {
				rt_kprintf("Failed to create service_gateway_sensor_config!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_sensor_config);	 

     //  2.11 SENSOR_CONFIG query
    service_gateway_config_sensor_query_t* service_gateway_sensor_config_query = service_gateway_config_sensor_query_create();
		if (service_gateway_sensor_config_query == NULL) {
				rt_kprintf("Failed to create service_gateway_sensor_config_query!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_sensor_config_query);	 

    //  2.12 DEVICE_CONFIG
    service_gateway_config_device_t* service_gateway_device_config = service_gateway_config_device_create();
		if (service_gateway_device_config == NULL) {
				rt_kprintf("Failed to create service_gateway_device_config!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_device_config);	 

    //  2.12 DEVICE_CONFIG query
    service_gateway_config_device_query_t* service_gateway_device_config_query = service_gateway_config_device_query_create();
		if (service_gateway_device_config_query == NULL) {
				rt_kprintf("Failed to create service_gateway_device_config_query!\n");
				return -RT_ENOMEM;				
		}			
    cps_list_add(&service_head_node, service_gateway_device_config_query);	 
    //  2.13 gateway_upgrade
    service_gateway_upgrade_t* service_gateway_upgrade = service_gateway_upgrade_create();
		if (service_gateway_upgrade == NULL) {
				rt_kprintf("Failed to create service_gateway_upgrade!\n");
				return -RT_ENOMEM;				
		}		    
    cps_list_add(&service_head_node, service_gateway_upgrade);	 

    //  2.14 gateway_config_task_list
    service_gateway_config_task_list_t* service_gateway_config_task_list = service_gateway_config_task_list_create();
		if (service_gateway_config_task_list == NULL) {
				rt_kprintf("Failed to create service_gateway_config_task_list!\n");
				return -RT_ENOMEM;				
		}		    
    cps_list_add(&service_head_node, service_gateway_config_task_list);	 


    //  2.14 gateway_config_task
    service_gateway_config_task_t* service_gateway_config_task = service_gateway_config_task_create();
		if (service_gateway_config_task == NULL) {
				rt_kprintf("Failed to create service_gateway_config_task!\n");
				return -RT_ENOMEM;				
		}		    
    cps_list_add(&service_head_node, service_gateway_config_task);	 
    
    //  2.15 gateway task query
    service_gateway_config_task_query_t* service_gateway_config_task_query = service_gateway_config_task_query_create();
		if (service_gateway_config_task_query == NULL) {
				rt_kprintf("Failed to create service_gateway_config_task_query!\n");
				return -RT_ENOMEM;				
		}		    
    cps_list_add(&service_head_node, service_gateway_config_task_query);	 
    
    //3.create and startup heartbeat_thread
    if(heartbeat_thread == RT_NULL){
        heartbeat_thread = rt_thread_create("heartbeat_thread", service_manager_heartbeat, RT_NULL, 768, 10, 30);
        if (heartbeat_thread != RT_NULL)
        {
            rt_err_t ret = rt_thread_startup(heartbeat_thread);							
						if (ret != RT_EOK) {
              rt_kprintf("failed to heartbeat_thread %d\n", ret);  
              rt_thread_delete(heartbeat_thread);
              heartbeat_thread = RT_NULL;
						} else {
              rt_kprintf("success to startup heartbeat_thread\r\n");
							is_service_udp_initialized = true;
						}
        }
    } 
    //3.create and startup period_report_thread
    /*if(period_report_thread == RT_NULL){
        period_report_thread = rt_thread_create("period_report_thread", service_manager_period_execute, RT_NULL, 2048, 10, 30);
        if (period_report_thread != RT_NULL)
        {
            rt_err_t ret = rt_thread_startup(period_report_thread);							
						if (ret != RT_EOK) {
              rt_kprintf("failed to period_report_thread %d\n", ret);  
              rt_thread_delete(period_report_thread);
              period_report_thread = RT_NULL;
						} else {
              rt_kprintf("success to startup period_report_thread\r\n");
							is_service_udp_initialized = true;
						}
        }
    }*/
		return RT_EOK;
}

rt_err_t service_manager_process_downlink(transport_udp_message_t *transport_udp_message, void* business_data){
    if(!is_service_udp_initialized){
        service_manager_udp_init();
    }

    service_t* service = query_service_by_type(transport_udp_message->transport_udp_data->service_type);
    if(!service){
        return RT_EINVAL;
    }
    
    return service->service_execute(transport_udp_message, business_data);
}

 /** @brief  Based on the input type (service type), traverse the service linked list pointed to by service_head_node, 
      find and return the first service_t instance with a matching type. If no matching instance is found, return NULL.
		 @Logic:
			cps_list_size(service_head_node): Retrieves the total number of nodes (total number of services) in the linked list.
			Traverse the linked list in a loop: Obtain the i-th service node by index through cps_list_get_node(service_head_node, i).
			Type matching check: Compare the node's service->type with the target type, and return the service pointer if they match.
 **/
service_t* query_service_by_type(const uint8_t type) {  
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
void service_manager_finalize(){
    cps_list_free(service_head_node);
}

INIT_APP_EXPORT(service_manager_udp_init);