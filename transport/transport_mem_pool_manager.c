#include "transport_mem_pool_manager.h"
#include <rtthread.h>
 rt_mp_t phy_payload_mpool 			= RT_NULL;
 rt_mp_t lora_message_mpool 		= RT_NULL;       
 rt_mp_t lora_payload_mpool 		= RT_NULL;
 rt_mp_t transport_udp_message_mpool = RT_NULL;  
 rt_mp_t transport_udp_payload_mpool = RT_NULL; 
 rt_mp_t transport_udp_header_mpool = RT_NULL; 
 rt_mp_t raw_udp_server_ip_mpool = RT_NULL;
 rt_mp_t transport_udp_service_data_mpool = RT_NULL; 

 rt_mp_t raw_udp_message_mpool 	= RT_NULL;  
 rt_mp_t raw_udp_payload_message_mpool 	= RT_NULL;
static void _delete_all_udp_pools(void)
{
    if (transport_udp_message_mpool != RT_NULL) {
        rt_mp_delete(transport_udp_message_mpool);
        transport_udp_message_mpool = RT_NULL;
    }
    if (transport_udp_payload_mpool != RT_NULL) {
        rt_mp_delete(transport_udp_payload_mpool);
        transport_udp_payload_mpool = RT_NULL;
    }
    if (transport_udp_header_mpool != RT_NULL) {
        rt_mp_delete(transport_udp_header_mpool);
        transport_udp_header_mpool = RT_NULL;
    }
    if (transport_udp_service_data_mpool != RT_NULL) {
        rt_mp_delete(transport_udp_service_data_mpool);
        transport_udp_service_data_mpool = RT_NULL;
    }
    if (raw_udp_message_mpool != RT_NULL) {
        rt_mp_delete(raw_udp_message_mpool);
        raw_udp_message_mpool = RT_NULL;
    }
    if (raw_udp_payload_message_mpool != RT_NULL) {
        rt_mp_delete(raw_udp_payload_message_mpool);
        raw_udp_payload_message_mpool = RT_NULL;
    }
}

int transport_mem_pool_manager_init(void){
    if (transport_udp_message_mpool == RT_NULL) {
        transport_udp_message_mpool = rt_mp_create(
            "udp_msg_pool", 
            UDP_DATA_POOL_NUM, 
            sizeof(transport_udp_message_t));
        if (transport_udp_message_mpool == RT_NULL) {
            rt_kprintf("[MEMPOOL] Failed to create transport_udp_message_mpool!\n");
            return -1;
        }
    }

    if (transport_udp_payload_mpool == RT_NULL) {
        transport_udp_payload_mpool = rt_mp_create(
            "udp_data_pool", 
            UDP_DATA_POOL_NUM, 
            sizeof(transport_udp_data_t));
        if (transport_udp_payload_mpool == RT_NULL) {
            rt_kprintf("[MEMPOOL] Failed to create transport_udp_payload_mpool!\n");
            _delete_all_udp_pools();
            return -1;
        }
    }

    if (transport_udp_header_mpool == RT_NULL) {
        transport_udp_header_mpool = rt_mp_create(
            "udp_hdr_pool", 
            UDP_DATA_POOL_NUM, 
            sizeof(transport_udp_header_t));
        if (transport_udp_header_mpool == RT_NULL) {
            rt_kprintf("[MEMPOOL] Failed to create transport_udp_header_mpool!\n");
            _delete_all_udp_pools();
            return -1;
        }
    }

    if (transport_udp_service_data_mpool == RT_NULL) {
        transport_udp_service_data_mpool = rt_mp_create(
            "udp_svc_pool", 
            UDP_DATA_POOL_NUM, 
            sizeof(service_data_t));
        if (transport_udp_service_data_mpool == RT_NULL) {
            rt_kprintf("[MEMPOOL] Failed to create transport_udp_service_data_mpool!\n");
            _delete_all_udp_pools();
            return -1;
        }
    }

    if (raw_udp_message_mpool == RT_NULL) {
        raw_udp_message_mpool = rt_mp_create(
            "raw_msg_pool", 
            UDP_DATA_POOL_NUM, 
            sizeof(raw_udp_message_t));
        if (raw_udp_message_mpool == RT_NULL) {
            rt_kprintf("[MEMPOOL] Failed to create raw_udp_message_mpool!\n");
            _delete_all_udp_pools();
            return RT_ERROR;
        }
    }

    if (raw_udp_payload_message_mpool == RT_NULL) {
        raw_udp_payload_message_mpool = rt_mp_create(
            "raw_pld_pool", 
            UDP_DATA_POOL_NUM, 
            UDP_MAX_PAYLOAD_LEN);
        if (raw_udp_payload_message_mpool == RT_NULL) {
            rt_kprintf("[MEMPOOL] Failed to create raw_udp_payload_message_mpool!\n");
            _delete_all_udp_pools();
            return RT_ERROR;
        }
    }

    rt_kprintf("[MEMPOOL] init OK (msg:%d hdr:%d data:%d svc:%d raw:%d pld:%d)\n",
        sizeof(transport_udp_message_t), sizeof(transport_udp_header_t),
        sizeof(transport_udp_data_t), sizeof(service_data_t),
        sizeof(raw_udp_message_t), UDP_MAX_PAYLOAD_LEN);
    return RT_EOK;
}

void request_mem_PHYPayload(PHYPayload_t* PHYPayload){
    PHYPayload = (PHYPayload_t*)rt_mp_alloc(phy_payload_mpool, RT_WAITING_FOREVER);
}

void release_mem_PHYPayload(PHYPayload_t* mem){
    rt_mp_free(mem);
}

void  request_mem_lora_message(lora_message_t** lora_message){
	 *lora_message = (lora_message_t *)rt_mp_alloc(lora_message_mpool, RT_WAITING_FOREVER);
}

void  release_mem_lora_message(lora_message_t* lora_message){
	rt_mp_free(lora_message);
}

void  request_mem_lora_payload(uint8_t** lora_payload){
	*lora_payload = (uint8_t *)rt_mp_alloc(lora_payload_mpool, RT_WAITING_FOREVER);
}

void  release_mem_lora_payload(uint8_t* lora_payload){
	rt_mp_free(lora_payload);
}

transport_udp_message_t* alloc_transport_udp_message(void)
{
    if (transport_udp_message_mpool == RT_NULL || transport_udp_header_mpool == RT_NULL ||
        transport_udp_payload_mpool == RT_NULL || transport_udp_service_data_mpool == RT_NULL) {
        rt_kprintf("[MEMPOOL] alloc_transport_udp_message: pools not initialized!\n");
        return RT_NULL;
    }

    transport_udp_message_t *msg = (transport_udp_message_t*)rt_mp_alloc(transport_udp_message_mpool, RT_WAITING_FOREVER);
    if (msg == RT_NULL) {
        rt_kprintf("[MEMPOOL] alloc transport_udp_message failed!\n");
        return RT_NULL;
    }
    rt_memset(msg, 0, sizeof(transport_udp_message_t));

    msg->transport_udp_header = (transport_udp_header_t*)rt_mp_alloc(transport_udp_header_mpool, RT_WAITING_FOREVER);
    if (msg->transport_udp_header == RT_NULL) {
        rt_kprintf("[MEMPOOL] alloc transport_udp_header failed!\n");
        rt_mp_free(msg);
        return RT_NULL;
    }
    rt_memset(msg->transport_udp_header, 0, sizeof(transport_udp_header_t));

    msg->transport_udp_data = (transport_udp_data_t*)rt_mp_alloc(transport_udp_payload_mpool, RT_WAITING_FOREVER);
    if (msg->transport_udp_data == RT_NULL) {
        rt_kprintf("[MEMPOOL] alloc transport_udp_data failed!\n");
        rt_mp_free(msg->transport_udp_header);
        rt_mp_free(msg);
        return RT_NULL;
    }
    rt_memset(msg->transport_udp_data, 0, sizeof(transport_udp_data_t));

    msg->service_data = (service_data_t*)rt_mp_alloc(transport_udp_service_data_mpool, RT_WAITING_FOREVER);
    if (msg->service_data == RT_NULL) {
        rt_kprintf("[MEMPOOL] alloc service_data failed!\n");
        rt_mp_free(msg->transport_udp_data);
        rt_mp_free(msg->transport_udp_header);
        rt_mp_free(msg);
        return RT_NULL;
    }
    rt_memset(msg->service_data, 0, sizeof(service_data_t));

    return msg;
}

void release_transport_udp_message(transport_udp_message_t* msg)
{
    if (msg == RT_NULL) return;

    if (msg->service_data != RT_NULL) {
        if (msg->service_data->business_data_payload != RT_NULL) {
            rt_free(msg->service_data->business_data_payload);
            msg->service_data->business_data_payload = RT_NULL;
        }
        if (msg->service_data->business_data != RT_NULL) {
            rt_free(msg->service_data->business_data);
            msg->service_data->business_data = RT_NULL;
        }
        rt_mp_free(msg->service_data);
        msg->service_data = RT_NULL;
    }
    if (msg->transport_udp_data != RT_NULL) {
        rt_mp_free(msg->transport_udp_data);
        msg->transport_udp_data = RT_NULL;
    }
    if (msg->transport_udp_header != RT_NULL) {
        rt_mp_free(msg->transport_udp_header);
        msg->transport_udp_header = RT_NULL;
    }
    rt_mp_free(msg);
}

raw_udp_message_t* alloc_raw_udp_message(void)
{
    if (raw_udp_message_mpool == RT_NULL || raw_udp_payload_message_mpool == RT_NULL) {
        rt_kprintf("[MEMPOOL] alloc_raw_udp_message: pools not initialized!\n");
        return RT_NULL;
    }

    raw_udp_message_t *msg = (raw_udp_message_t*)rt_mp_alloc(raw_udp_message_mpool, RT_WAITING_FOREVER);
    if (msg == RT_NULL) {
        rt_kprintf("[MEMPOOL] alloc raw_udp_message failed!\n");
        return RT_NULL;
    }
    rt_memset(msg, 0, sizeof(raw_udp_message_t));

    msg->payload = (char*)rt_mp_alloc(raw_udp_payload_message_mpool, RT_WAITING_FOREVER);
    if (msg->payload == RT_NULL) {
        rt_kprintf("[MEMPOOL] alloc raw_udp_payload failed!\n");
        rt_mp_free(msg);
        return RT_NULL;
    }
    rt_memset(msg->payload, 0, UDP_MAX_PAYLOAD_LEN);
    msg->payload_length = 0;
    return msg;
}

void release_raw_udp_message(raw_udp_message_t* msg)
{
    if (msg == RT_NULL) return;
    if (msg->payload != RT_NULL) {
        rt_mp_free(msg->payload);
        msg->payload = RT_NULL;
    }
    rt_mp_free(msg);
}		

INIT_COMPONENT_EXPORT(transport_mem_pool_manager_init);