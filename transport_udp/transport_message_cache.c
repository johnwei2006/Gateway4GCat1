#include "transport_message_cache.h"
#include <rtthread.h>
#include "../board/board.h"
#include "../transport/transport_mem_pool_manager.h"

#define SRAM_BASE_ADDR  0x20000000
#define SRAM_END_ADDR   STM32_SRAM_END

static bool     is_initialized = false;
struct cps_list_node *uplink_message_cache_head_node = RT_NULL;
rt_uint8_t current_resend_uplink_message_index = 0;
rt_mutex_t uplink_message_cache_mutex = RT_NULL;

rt_err_t put_uplink_message_cache(transport_udp_message_t *transport_udp_message){
    uint8_t list_size = cps_list_size(uplink_message_cache_head_node);
     rt_kprintf("__________________________________________________________________\r\n");
    rt_kprintf("before to put_uplink_message_cache: current list_size: %d \r\n", list_size);
    rt_kprintf("put_uplink_message_cache: message_id: %d \r\n", transport_udp_message->transport_udp_data->message_id);
    rt_mutex_take(uplink_message_cache_mutex, RT_WAITING_FOREVER);
    cps_list_add(&uplink_message_cache_head_node, transport_udp_message);
    
    list_size = cps_list_size(uplink_message_cache_head_node);
    rt_kprintf("after to put_uplink_message_cache: current list_size: %d \r\n", list_size);
    rt_mutex_release(uplink_message_cache_mutex);
    return RT_EOK;
}

rt_err_t query_uplink_message_cache_by_message_id(transport_udp_message_t *transport_udp_message, rt_uint32_t message_id){
    uint8_t list_size = cps_list_size(uplink_message_cache_head_node);
    for(uint8_t i = 1; i <= list_size; i++){
        transport_udp_message_t *temp_transport_udp_message = (transport_udp_message_t *)cps_list_get_node(uplink_message_cache_head_node, i);
        if(temp_transport_udp_message->transport_udp_data->message_id == message_id){
            transport_udp_message = temp_transport_udp_message;
            return RT_EOK;
        }
    }
		
    return RT_EINVAL;
}

rt_err_t remove_uplink_message_cache(rt_uint32_t message_id){
    rt_mutex_take(uplink_message_cache_mutex, RT_WAITING_FOREVER);
    uint8_t list_size = cps_list_size(uplink_message_cache_head_node);
    rt_kprintf("__________________________________________________________________\r\n");
    rt_kprintf("before to remove_uplink_message_cache: list_size: %d \r\n", list_size);
    for(uint8_t i = 1; i <= list_size; i++){
        transport_udp_message_t *temp_transport_udp_message = (transport_udp_message_t *)cps_list_get_node(uplink_message_cache_head_node, i);
        rt_kprintf("number %d cache message_id: %d\r\n", i, temp_transport_udp_message->transport_udp_data->message_id);
        if(temp_transport_udp_message->transport_udp_data->message_id == message_id){
            rt_kprintf("remove_uplink_message_cache: message_id: %d, found \r\n", message_id);
            cps_list_node_t* deleted_node = cps_list_delete(&uplink_message_cache_head_node, i);
            if(deleted_node != RT_NULL){
                temp_transport_udp_message = (transport_udp_message_t *)deleted_node->data;
                rt_free(deleted_node);
                deleted_node = RT_NULL;
                free_transport_udp_message(temp_transport_udp_message);
            }
            break;
        }
    }
    list_size = cps_list_size(uplink_message_cache_head_node);
      rt_kprintf("after to remove_uplink_message_cache: list_size: %d \r\n", list_size);
    rt_mutex_release(uplink_message_cache_mutex);
    return RT_EOK;
}

transport_udp_message_t* get_next_uplink_message_cache(void){
    rt_mutex_take(uplink_message_cache_mutex, RT_WAITING_FOREVER);
    if(current_resend_uplink_message_index >= cps_list_size(uplink_message_cache_head_node)){
        current_resend_uplink_message_index = 0;
    }
    current_resend_uplink_message_index++;
    transport_udp_message_t *temp_transport_udp_message = (transport_udp_message_t *)cps_list_get_node(uplink_message_cache_head_node, current_resend_uplink_message_index);
    rt_mutex_release(uplink_message_cache_mutex);
    return temp_transport_udp_message;
}

rt_err_t free_transport_udp_message(transport_udp_message_t *transport_udp_message) {
    if (transport_udp_message == RT_NULL) 
        return RT_EINVAL;

    if (transport_udp_message->service_data != RT_NULL) {
        if(transport_udp_message->service_data->business_data_payload != RT_NULL){
            rt_free(transport_udp_message->service_data->business_data_payload);
            transport_udp_message->service_data->business_data_payload = RT_NULL;
        }
        if(transport_udp_message->service_data->business_data != RT_NULL){
            rt_free(transport_udp_message->service_data->business_data);
            transport_udp_message->service_data->business_data = RT_NULL;
        }
        if ((uint32_t)transport_udp_message->service_data >= SRAM_BASE_ADDR && 
            (uint32_t)transport_udp_message->service_data < SRAM_END_ADDR) {
            rt_free(transport_udp_message->service_data);
        } else {
            rt_kprintf("Error: service_data invalid ptr: 0x%08X\n", 
                       (uint32_t)transport_udp_message->service_data);
        }
        transport_udp_message->service_data = RT_NULL;
    }
    if (transport_udp_message->transport_udp_data != RT_NULL) {
        if ((uint32_t)transport_udp_message->transport_udp_data >= SRAM_BASE_ADDR && 
            (uint32_t)transport_udp_message->transport_udp_data < SRAM_END_ADDR) {
            rt_free(transport_udp_message->transport_udp_data);
        } else {
            rt_kprintf("Error: transport_udp_data invalid ptr: 0x%08X\n", 
                       (uint32_t)transport_udp_message->transport_udp_data);
        }
        transport_udp_message->transport_udp_data = RT_NULL;
    }
    if (transport_udp_message->transport_udp_header != RT_NULL) {
        if ((uint32_t)transport_udp_message->transport_udp_header >= SRAM_BASE_ADDR && 
            (uint32_t)transport_udp_message->transport_udp_header < SRAM_END_ADDR) {
            rt_free(transport_udp_message->transport_udp_header);
        } else {
            rt_kprintf("Error: transport_udp_header invalid ptr: 0x%08X\n", 
                       (uint32_t)transport_udp_message->transport_udp_header);
        }
        transport_udp_message->transport_udp_header = RT_NULL;
    }
    if ((uint32_t)transport_udp_message >= SRAM_BASE_ADDR && (uint32_t)transport_udp_message < SRAM_END_ADDR) {
        rt_free(transport_udp_message);
    } else {
        rt_kprintf("Error: free_transport_udp_message invalid pointer: 0x%08X\n", (uint32_t)transport_udp_message);
        return RT_EINVAL;
    }
    return RT_EOK;
}
rt_err_t free_raw_udp_message(raw_udp_message_t *raw_udp_message){
    if (raw_udp_message == RT_NULL) 
        return RT_EINVAL;
    if (raw_udp_message->payload != RT_NULL) {
        rt_free(raw_udp_message->payload);
        raw_udp_message->payload = RT_NULL;
    }
    rt_free(raw_udp_message);
    raw_udp_message = RT_NULL;
    return RT_EOK;
}
int transport_message_cache_init(void)
{
    if(is_initialized){
        return RT_EOK;
    }   
    uplink_message_cache_mutex = rt_mutex_create("uplink_message_cache_mutex", RT_IPC_FLAG_FIFO);
    if(uplink_message_cache_mutex == RT_NULL){
        rt_kprintf("create message cache mutex failed\n");
        return -1;
    }
    uplink_message_cache_head_node = (struct cps_list_node *)cps_list_init();

    is_initialized = true;
    return RT_EOK;
}

uint16_t get_uplink_message_cache_size(void)
{
    uint16_t size = 0;
    //rt_mutex_take(uplink_message_cache_mutex, RT_WAITING_FOREVER);
    size = cps_list_size(uplink_message_cache_head_node);
    //rt_mutex_release(uplink_message_cache_mutex);
    return size;
}

INIT_COMPONENT_EXPORT(transport_message_cache_init);