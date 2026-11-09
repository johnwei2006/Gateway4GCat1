#ifndef TRANSPORT_MESSAGE_CACHE_H_
#define TRANSPORT_MESSAGE_CACHE_H_ 

#include  <rtthread.h>
#include "../utils/cps_list.h"
#include "../transport_udp/transport_udp_message.h"
#include <stdbool.h>

extern struct   cps_list_node *uplink_message_cache_head_node;
extern rt_uint8_t current_resend_uplink_message_index;
extern rt_mutex_t uplink_message_cache_mutex;

rt_err_t put_uplink_message_cache(transport_udp_message_t *transport_udp_message);

rt_err_t query_uplink_message_cache_by_message_id(transport_udp_message_t *transport_udp_message, rt_uint32_t message_id);  

rt_err_t remove_uplink_message_cache(rt_uint32_t message_id);

transport_udp_message_t* get_next_uplink_message_cache(void);

int transport_message_cache_init(void);

rt_err_t free_transport_udp_message(transport_udp_message_t *transport_udp_message);

rt_err_t free_raw_udp_message(raw_udp_message_t *raw_udp_message);

uint16_t get_uplink_message_cache_size(void);

#endif /*TRANSPORT_MESSAGE_CACHE_H_*/