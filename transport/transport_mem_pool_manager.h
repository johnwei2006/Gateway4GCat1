#ifndef TRANSPORT_MEM_POOL_MANAGER_H_
#define TRANSPORT_MEM_POOL_MANAGER_H_

#include  <rtthread.h>
#include "../transport_lora/lora_message.h"
#include "../transport_udp/transport_udp_message.h"

#define MEM_POOL_SIZE 8
#define UDP_DATA_POOL_NUM 4 
#define UDP_MAX_PAYLOAD_LEN 1024
#define PHY_MSG_POOL_NUM 3
extern rt_mp_t phy_payload_mpool; 		
extern rt_mp_t lora_message_mpool;	     
extern rt_mp_t lora_payload_mpool;
extern rt_mp_t transport_udp_message_mpool;  
extern rt_mp_t transport_udp_payload_mpool;;
extern rt_mp_t transport_udp_header_mpool; 
extern rt_mp_t transport_udp_service_data_mpool; 

extern rt_mp_t raw_udp_message_mpool;  
extern rt_mp_t raw_udp_payload_message_mpool;

int  transport_mem_pool_manager_init(void);

extern void  request_mem_PHYPayload(PHYPayload_t* mem); 
extern void  release_mem_PHYPayload(PHYPayload_t* mem);
 
extern void  request_mem_lora_message(lora_message_t** lora_message); 
extern void  release_mem_lora_message(lora_message_t* lora_message);

extern void  request_mem_lora_payload(uint8_t** lora_payload); 
extern void  release_mem_lora_payload(uint8_t* lora_payload);

extern transport_udp_message_t* alloc_transport_udp_message(void);
extern void release_transport_udp_message(transport_udp_message_t* msg);

extern raw_udp_message_t* alloc_raw_udp_message(void);
extern void release_raw_udp_message(raw_udp_message_t* msg);
#endif /*TRANSPORT_MEM_POOL_MANAGER_H_*/