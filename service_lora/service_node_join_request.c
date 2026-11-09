#include "../transport/transport_message_queue.h"
#include "../transport_lora/lora_message.h"
#include "service_node_join_request.h"
#include "../utils/utils.h"
#include "../config/config_manager.h"

void service_execute_node_join_request(void* p_data){
    //1. get node_join_request_data
    PHYPayload_t *PHYPayload = (PHYPayload_t *)p_data;
    node_join_request_data_t *node_join_request_data = (node_join_request_data_t *)PHYPayload->mac_payload->service_data;
    

    //2、generate udp uplink message
		transport_udp_message_t* gateway_uplink_message  	= (transport_udp_message_t*)rt_malloc(sizeof(transport_udp_message_t));
		if(gateway_uplink_message == NULL)
		{
				rt_kprintf("memory allocate gateway_uplink_message failed. \n");
        return;
		}
		rt_memset(gateway_uplink_message, 0, sizeof(transport_udp_message_t));
		
		gateway_uplink_message->transport_udp_header	= (transport_udp_header_t*)rt_malloc(sizeof(transport_udp_header_t));
		gateway_uplink_message->transport_udp_data		= (transport_udp_data_t*)rt_malloc(sizeof(transport_udp_data_t));
		gateway_uplink_message->service_data					= (service_data_t*)rt_malloc(sizeof(service_data_t));
		
    rt_memset(gateway_uplink_message->transport_udp_header, 0, sizeof(transport_udp_header_t));
    rt_memset(gateway_uplink_message->transport_udp_data, 	0, sizeof(transport_udp_data_t));
    rt_memset(gateway_uplink_message->service_data, 				0, sizeof(service_data_t));
        
    rt_memcpy(gateway_uplink_message->transport_udp_data->node_id, PHYPayload->mac_payload->node_id, 4);
		gateway_uplink_message->transport_udp_data->event_id    		  = generate_rand_short();
		gateway_uplink_message->transport_udp_data->service_type  		= SERVICE_TYPE_NODE_JOIN_REQUEST;
    gateway_uplink_message->transport_udp_data->service_version   = SERVICE_VERSION_1; 
    gateway_uplink_message->service_data->business_data 				  = node_join_request_data;
    gateway_uplink_message->service_data->business_data_payload 	= RT_NULL;
    gateway_uplink_message->service_data->business_data_payload_length= 0;
        
    rt_kprintf("service_execute_node_join_request executed, event_id: %d \n", gateway_uplink_message->transport_udp_data->event_id);
		//3. send to uplink message queue
		put_gateway_uplink_message(&gateway_uplink_message);
};

service_node_join_request_t* service_node_join_request_create(){
    service_node_join_request_t* service = (service_node_join_request_t*)rt_malloc(sizeof(service_node_join_request_t));
    service->base.service_type      = FCMD_NODE_JOIN_REQUEST;
    service->base.service_execute   = service_execute_node_join_request;
    return service;
}