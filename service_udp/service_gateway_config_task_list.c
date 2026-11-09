#include "service_gateway_config_task_list.h"
#include "../config/config_manager.h"


rt_err_t service_execute_gateway_config_task_list(void* transport_udp_message, void* business_data){
    if(transport_udp_message == RT_NULL)
        return RT_EINVAL;
    //1. query current task configurations from flash
    //2. add new task configurations from the received data

    transport_udp_message_t *transport_udp_message_ptr = (transport_udp_message_t*)transport_udp_message;
    config_gateway_task_list_t *config_task_list = RT_NULL;
    get_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id, (void**)&config_task_list);
    if(config_task_list == RT_NULL){
        rt_kprintf("Error: get_business_data_cache failed, event_id: %d\n", transport_udp_message_ptr->transport_udp_data->event_id);
        return RT_EINVAL;
    }
    rt_kprintf("Info: Updating gateway task configuration...\n");
    rt_kprintf("Info: Task Size: %d\n", config_task_list->task_count);

    //1. query current task configurations from flash and delete them
    config_gateway_task_list_t existing_config_task_list = {0};
    query_config_task_list(&existing_config_task_list);
    for(int i = 0; i < existing_config_task_list.task_count; i++){
        rt_kprintf("Info: Existing Task %d: %s\n", i, existing_config_task_list.config_tasks[i].task_id);
        delete_config_task(&existing_config_task_list.config_tasks[i]);
    }
    //2. add new task configurations from the received data
    flash_status status;
    service_data_config_response_t* response = (service_data_config_response_t*)rt_malloc(sizeof(service_data_config_response_t));
    for(int i = 0; i < config_task_list->task_count; i++){
        rt_kprintf("Info: Task %d: %s\n", i, config_task_list->config_tasks[i].task_id);
        status = add_or_update_config_task(&config_task_list->config_tasks[i]);
        if(status != FLASH_OK){
            response->result = status;
            strcpy(response->message, "updated config task failed.");
            goto exit;
        }
    }
    response->result = RT_EOK;
    strcpy(response->message, "updated config task success.");
exit:
		remove_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id);
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
        
		gateway_uplink_message->transport_udp_data->event_id    		= transport_udp_message_ptr->transport_udp_data->event_id; //generate_rand_short();
		gateway_uplink_message->transport_udp_data->service_type  	= SERVICE_TYPE_GATEWAY_CONFIG_TASK_LIST_RESPONSE;
		gateway_uplink_message->transport_udp_data->service_version = SERVICE_VERSION_1; 
		gateway_uplink_message->service_data->business_data_payload = RT_NULL;
		gateway_uplink_message->service_data->business_data_payload_length= 0;
		gateway_uplink_message->service_data->business_data 				= RT_NULL;

		put_business_data_cache(gateway_uplink_message->transport_udp_data->event_id, response);
    rt_err_t result = put_gateway_uplink_message(&gateway_uplink_message);
    if(result != RT_EOK){
      rt_kprintf("Warning: put_gateway_uplink_message failed, free memory\n");
      free_transport_udp_message(gateway_uplink_message);  // 正确释放内存
      return result;
    }        
		return RT_EOK;
};

service_gateway_config_task_list_t* service_gateway_config_task_list_create(void){
    service_gateway_config_task_list_t* service_gateway_config_task_list = (service_gateway_config_task_list_t*)rt_malloc(sizeof(service_gateway_config_task_list_t));
    service_gateway_config_task_list->base.service_type 		= SERVICE_TYPE_GATEWAY_CONFIG_TASK_LIST;
    service_gateway_config_task_list->base.service_execute 	= service_execute_gateway_config_task_list;
    return service_gateway_config_task_list;
}