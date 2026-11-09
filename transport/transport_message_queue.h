#ifndef TRANSPORT_MESSAGE_QUEUE_H_
#define TRANSPORT_MESSAGE_QUEUE_H_

#include  <rtthread.h>
#include "../transport_lora/lora_message.h"
#include "../transport_udp/transport_udp_message.h"

#define CLOUD_MSG_MAX_LENGTH 10
#define CLOUD_MSG_SIZE 1024

/*

1、downlink:
gateway -> node_downlink_message -> lora_downlink_message -> node
2、uplink:
node    -> lora_uplink_message   -> node_uplink_message   -> gateway

lora_uplink_message				:lora_message_t
lora_downlink_message			:lora_message_t
--------------lora_encode/lora_decode--------------
node_uplink_message			    :PHYPayload_t
node_downlink_message   		:PHYPayload_t


gateway_uplink_message			:transport_udp_message_t
gateway_downlink_message		:transport_udp_message_t
---------------transport_udp_encode/transport_udp_decode--------------
cloud_uplink_message			:raw_udp_message_t
cloud_downlink_message			:raw_udp_message_t
*/

//1. lora message queue
extern rt_mq_t 		lora_uplink_mq;
/*Message queue (used to cache pending downlink messages)*/
extern rt_mq_t 		lora_downlink_mq;

/*Gateway business logic functions (functional encapsulation)*/
rt_err_t put_lora_uplink_message(lora_message_t *lora_rx_message);
rt_err_t get_lora_uplink_message(lora_message_t **lora_rx_message);

rt_err_t put_lora_downlink_message(lora_message_t *lora_tx_message);
rt_err_t get_lora_downlink_message(lora_message_t **lora_tx_message);

//2. node message queue
extern rt_mq_t 		node_downlink_mq;

rt_err_t put_node_downlink_message(PHYPayload_t **PHYPayload);
rt_err_t get_node_downlink_message(PHYPayload_t **PHYPayload);

//3. gateway message queue
extern rt_mq_t 		gateway_uplink_mq;
extern rt_mq_t 		gateway_downlink_mq;

rt_err_t put_gateway_uplink_message(transport_udp_message_t **gateway_uplink_message);
rt_err_t get_gateway_uplink_message(transport_udp_message_t **gateway_uplink_message);

rt_err_t put_gateway_downlink_message(transport_udp_message_t *gateway_downlink_message);
rt_err_t get_gateway_downlink_message(transport_udp_message_t **gateway_downlink_message);

//4. cloud message queue
extern rt_mq_t 		cloud_uplink_mq;
extern rt_mq_t 		cloud_downlink_mq;

rt_err_t put_cloud_uplink_message(raw_udp_message_t **raw_udp_message);
rt_err_t get_cloud_uplink_message(raw_udp_message_t **raw_udp_message);

rt_err_t put_cloud_downlink_message(raw_udp_message_t *raw_udp_message);
rt_err_t get_cloud_downlink_message(raw_udp_message_t **raw_udp_message);

int transport_message_queue_init(void);

#endif /*TRANSPORT_MESSAGE_QUEUE_H_*/