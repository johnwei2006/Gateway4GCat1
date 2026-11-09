#include "service_gateway_config_node.h"
#include "../gateway_manager/rtc_manager.h"

static service_gateway_config_node_t service_gateway_config_node_static;
rt_err_t service_execute_gateway_config_node(void* transport_udp_message, void* business_data){
    if(transport_udp_message == RT_NULL)
        return RT_EINVAL;
    //1. query current node configurations from flash
    //2. update existing node configurations with received data
    //3. delete node configurations(including sensors & devices) that are not in the received data
    //4. add new node configurations from the received data

    transport_udp_message_t *transport_udp_message_ptr = (transport_udp_message_t*)transport_udp_message;
    service_data_config_node_t *gateway_node_config = RT_NULL;
    get_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id, (void**)&gateway_node_config);
    if(gateway_node_config == RT_NULL){
        rt_kprintf("Info: gateway_node_config is NULL\n");
        return RT_EINVAL;
    }
    rt_kprintf("Info: Updating gateway node configuration...\n");
    rt_kprintf("Info: Node Size: %d\n", gateway_node_config->node_size);

    config_node_list_t *node_config_list = (config_node_list_t*)rt_calloc(1, sizeof(config_node_list_t));
    if (node_config_list == RT_NULL) {
        rt_kprintf("Error: alloc node_config_list failed\n");
        return RT_ENOMEM;
    }
    node_config_list->nodes_count = gateway_node_config->node_size;
    for(uint8_t i = 0; i < gateway_node_config->node_size; i++){
        rt_memcpy(node_config_list->config_nodes[i].node_id, gateway_node_config->node_config_items[i].node_id, NODE_ID_LENGTH);
        node_config_list->config_nodes[i].node_type = gateway_node_config->node_config_items[i].node_type;
        node_config_list->config_nodes[i].report_interval = gateway_node_config->node_config_items[i].report_interval;
        node_config_list->config_nodes[i].lora_freq_group = gateway_node_config->node_config_items[i].lora_freq_group;
        //rt_free(&gateway_node_config->node_config_items[i]);
        //gateway_node_config->node_config_items[i] = RT_NULL;
    }
    rt_device_rtc_time_t rtc_time = {0};
    if (read_rtc_time(&rtc_time) == 0) {
        node_config_list->config_time[0] = rtc_time.year % 100;
        node_config_list->config_time[1] = rtc_time.month;
        node_config_list->config_time[2] = rtc_time.day;
        node_config_list->config_time[3] = rtc_time.hour;
        node_config_list->config_time[4] = rtc_time.min;
        node_config_list->config_time[5] = rtc_time.sec;
        for (uint8_t i = 0; i < gateway_node_config->node_size; i++) {
            rt_memcpy(node_config_list->config_nodes[i].config_time, node_config_list->config_time, sizeof(node_config_list->config_time));
        }
    } else {
        rt_kprintf("Error: Failed to read RTC time\n");
    }    
    rt_free(gateway_node_config->node_config_items);

    service_data_config_response_t* response = (service_data_config_response_t*)rt_malloc(sizeof(service_data_config_response_t));
    flash_status status = update_config_node(node_config_list);
    rt_free(node_config_list);
    
    if(status != FLASH_OK){
       rt_kprintf("Error: update_config_node failed, status: %d\n", status);
       response->result = status;
       strcpy(response->message, "updated config node failed.");
       goto exit;
    }
    response->result = RT_EOK;
    strcpy(response->message, "updated config node success.");

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
		gateway_uplink_message->transport_udp_data->service_type  	= SERVICE_TYPE_GATEWAY_CONFIG_NODE_RESPONSE;
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

service_gateway_config_node_t* service_gateway_config_node_create(void){
    rt_memset(&service_gateway_config_node_static, 0, sizeof(service_gateway_config_node_t));
    service_gateway_config_node_static.base.service_type    = SERVICE_TYPE_GATEWAY_CONFIG_NODE;
    service_gateway_config_node_static.base.service_execute = service_execute_gateway_config_node;
    return &service_gateway_config_node_static;
}