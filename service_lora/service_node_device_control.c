#include "../transport/transport_message_queue.h"
#include "../transport_lora/lora_message.h"
#include "service_node_device_control.h"
#include "../utils/utils.h"
#include "../config/config_manager.h"
#include "../gateway_manager/gateway_manager.h"

rt_err_t service_execute_node_device_control(void* p_data){
    transport_udp_message_t *transport_udp_message = (transport_udp_message_t*)p_data;

    node_device_control_t *node_device_control = (node_device_control_t *)transport_udp_message->service_data->business_data;
    uint8_t device_control_size = (transport_udp_message->service_data->business_data_payload_length - 4)/sizeof(node_device_control_t);

    PHYPayload_t* PHYPayload;
    //1、set message_id
    short message_id = generate_rand_short();
    PHYPayload->mac_payload->message_id[0] = message_id & 0xFF;
    PHYPayload->mac_payload->message_id[1] = (message_id >> 8) & 0xFF;

    PHYPayload->mac_payload->fcmd = FCMD_NODE_DEVICE_CONTROL;

    PHYPayload->mac_payload->flen = 0;

    //3、set mac.frm_payload
    for(uint8_t i = 0; i < device_control_size; i++){
        node_device_control_t control = node_device_control[i];
        PHYPayload->mac_payload->frm_payload[i*2]     = control.device_id;
        PHYPayload->mac_payload->frm_payload[i*2+1]   = control.action;
    }
    
    put_node_downlink_message(&PHYPayload);
    return RT_EOK;
}

service_node_device_control_t* service_node_device_control_create(){
    service_node_device_control_t* service = (service_node_device_control_t*)rt_malloc(sizeof(service_node_device_control_t));
    service->base.service_type      = FCMD_NODE_DEVICE_CONTROL;
    service->base.service_execute   = service_execute_node_device_control;
    return service;
}