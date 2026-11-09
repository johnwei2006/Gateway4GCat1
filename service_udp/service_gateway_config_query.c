#include "service_gateway_config_query.h"

service_gateway_config_query_t* service_gateway_config_query_create(){
    service_gateway_config_query_t* service_gateway_config_query = (service_gateway_config_query_t*)rt_malloc(sizeof(service_gateway_config_query_t));
    service_gateway_config_query->base.service_type 		= SERVICE_TYPE_GATEWAY_CONFIG_QUERY;
    service_gateway_config_query->base.service_execute 	    = service_execute_gateway_config_query;
    return service_gateway_config_query;
}

rt_err_t service_execute_gateway_config_query(void* transport_udp_message, void* business_data) {
    rt_kprintf("Querying gateway configuration...\n");
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
        
		gateway_uplink_message->transport_udp_data->event_id    		= ((transport_udp_message_t*)transport_udp_message)->transport_udp_data->event_id;
		gateway_uplink_message->transport_udp_data->service_type  	= SERVICE_TYPE_GATEWAY_CONFIG_REPORT;
    gateway_uplink_message->transport_udp_data->service_version = SERVICE_VERSION_1;     

    config_gateway_serial_t *gateway_config = (config_gateway_serial_t*)rt_malloc(sizeof(config_gateway_serial_t));
    flash_status status = read_config_gateway_serial(gateway_config);
    if (status != FLASH_OK) {
        free_transport_udp_message(gateway_uplink_message);
        rt_free(gateway_config);      
        rt_kprintf("read gateway config failed. \n");
        return RT_EIO;
    }
    if(gateway_config->gateway_id == RT_NULL){
        rt_kprintf("Error: Gateway config is NULL\n");
        free_transport_udp_message(gateway_uplink_message);
        rt_free(gateway_config);
        return -RT_EIO;
    }
    
    put_business_data_cache(gateway_uplink_message->transport_udp_data->event_id, gateway_config);
    rt_kprintf("Gateway config report service executed. \n");
		// 发送消息
		put_gateway_uplink_message(&gateway_uplink_message);    
		return RT_EOK;
};