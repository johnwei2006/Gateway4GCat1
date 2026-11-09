#include "service_gateway_config_node_query.h"

rt_err_t service_execute_gateway_config_node_query(void* transport_udp_message, void* business_data){
	transport_udp_message_t* gateway_uplink_message  	= (transport_udp_message_t*)rt_malloc(sizeof(transport_udp_message_t));
	if(gateway_uplink_message == NULL)
	{
		rt_kprintf("memory allocate gateway_uplink_message failed. \n");
    return RT_ENOMEM;
	}
	rt_memset(gateway_uplink_message, 0, sizeof(transport_udp_message_t));
		
	gateway_uplink_message->transport_udp_header			= (transport_udp_header_t*)rt_malloc(sizeof(transport_udp_header_t));
	gateway_uplink_message->transport_udp_data				= (transport_udp_data_t*)rt_malloc(sizeof(transport_udp_data_t));
	gateway_uplink_message->service_data					    = (service_data_t*)rt_malloc(sizeof(service_data_t));
		
  rt_memset(gateway_uplink_message->transport_udp_header, 0, sizeof(transport_udp_header_t));
  rt_memset(gateway_uplink_message->transport_udp_data, 	0, sizeof(transport_udp_data_t));
  rt_memset(gateway_uplink_message->service_data, 		    0, sizeof(service_data_t));
        
	gateway_uplink_message->transport_udp_data->event_id    	  = ((transport_udp_message_t*)transport_udp_message)->transport_udp_data->event_id;
	gateway_uplink_message->transport_udp_data->service_type  	= SERVICE_TYPE_GATEWAY_CONFIG_NODE_REPORT;
  gateway_uplink_message->transport_udp_data->service_version = SERVICE_VERSION_1; 
  gateway_uplink_message->service_data->business_data_payload = RT_NULL;
  gateway_uplink_message->service_data->business_data_payload_length= 0;
  gateway_uplink_message->service_data->business_data 		    = RT_NULL;
  
  config_node_list_t *config_node_list = (config_node_list_t*)rt_malloc(sizeof(config_node_list_t));
  flash_status status = query_config_node_list(config_node_list);
  if(status != FLASH_OK){
    rt_kprintf("Error: query_config_node_list failed, status: %d\n", status);
    free_transport_udp_message(gateway_uplink_message);
    rt_free(config_node_list);
    return RT_ERROR;
  }
  
  put_business_data_cache(gateway_uplink_message->transport_udp_data->event_id, config_node_list);
  rt_kprintf("Gateway node config query service executed. \n");
	// 发送消息
	put_gateway_uplink_message(&gateway_uplink_message);
  return RT_EOK;
};

service_gateway_config_node_query_t* service_gateway_config_node_query_create(void){
    service_gateway_config_node_query_t* service_gateway_config_node_query = (service_gateway_config_node_query_t*)rt_malloc(sizeof(service_gateway_config_node_query_t));
    if(service_gateway_config_node_query == RT_NULL){
        rt_kprintf("Error: Failed to allocate service_gateway_config_node_query\n");
        return RT_NULL;
    }
    rt_memset(service_gateway_config_node_query, 0, sizeof(service_gateway_config_node_query_t));

    service_gateway_config_node_query->base.service_type 		= SERVICE_TYPE_GATEWAY_CONFIG_NODE_QUERY;
    service_gateway_config_node_query->base.service_execute 	= service_execute_gateway_config_node_query;
    return service_gateway_config_node_query;
}

