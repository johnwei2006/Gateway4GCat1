#include "lora_encoder.h"
#include <rtthread.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "utils.h"
#include "lora_handler.h"
#include "lora_service_encoder.h"
#include "../transport/transport_message_queue.h"
#include "../transport_lora/lora_message.h"

/**
 * @brief Parse the LoRa received message into a PHYPayload structure
 * @param rx_message Received LoRa message
 * @param phy_payload Output PHYPayload structure
 * @return RT_EOK Success, other error codes indicate failure
 * @return liujianfeng 2025 08
 */

int lora_decode(lora_message_t *rx_message, PHYPayload_t *PHYPayload)
	{
	  uint16_t offset = 0;
	 // MACPayload_t macpayload={0};   
    uint8_t mac_payload_length = rx_message->payload_length - 3;
	// Parameter validity check
	if (rx_message == RT_NULL || PHYPayload == RT_NULL || 
        rx_message->payload == RT_NULL || rx_message->payload_length == 0)
    {
        rt_kprintf("Parsing failed: Invalid parameter\n");
        return -RT_EINVAL;
    }	
    PHYPayload->mlen=rx_message->payload[offset];
//	  rt_memset(rx_message, 0, sizeof(PHYPayload_t)); // Initialize the PHYPayload structure to zero.	
		
    /* Acquire the mutex lock to ensure that data is not tampered with during the
		   parsing process (especially in a multi-threaded environment)*/
		
//    if (rt_mutex_take(lora_mutex, RT_WAITING_FOREVER) != RT_EOK)
//    {
//        rt_kprintf("Parsing failed: Timeout when acquiring mutex lock\n");
//        return -RT_ETIMEOUT;
//    }
    if(rx_message->payload_length < LORA_PAYLOAD_FIX_SIZE){
			  rt_kprintf("Parsing failed: Insufficient data length\n");        
        return -1;
    } else {   	
		PHYPayload->mhdr.mtype = (rx_message->payload[offset] >> 5) & 0x07; 
				switch (PHYPayload->mhdr.mtype){
					case  join_req:  rt_kprintf("join_req");break;
          case  join_apt:  rt_kprintf("join_apt") ;break;
		      case  unconfirmed_data_up:  rt_kprintf("unconfirmed data up");break;
					case  unconfirmed_down_up:  rt_kprintf("unconfirmed down up");break;
					case  confirmed_data_up:rt_kprintf("unconfirmed down up");break;
					case  confirmed_data_down:	rt_kprintf("unconfirmed down up");break;
          case  ping_accept : rt_kprintf("ping accept");break;
          default:
                break;
        }
	      PHYPayload->mhdr.protocol_version = rx_message->payload[offset] & 0x07;  
				switch (PHYPayload->mhdr.protocol_version){
					case  LoRaWAN1_0x:   rt_kprintf("LoRaWAN10x");break;
					case  LoRaWAN1_10:   rt_kprintf("LoRaWAN110");break;
					default:
                break;
				}
				PHYPayload->mhdr.upgrade=(rx_message->payload[offset] >> 4) & 0x01;
        switch (PHYPayload->mhdr.upgrade){
					case  ordinary_packet:   rt_kprintf("ordinary packet\n");break;  
					case  upgarade_packet:   rt_kprintf("upgarade packet\n");break;
				}	
        PHYPayload->mhdr.broadcast=(rx_message->payload[offset] >> 3) & 0x01;
        switch (PHYPayload->mhdr.broadcast){
					case  non_broadcast:   rt_kprintf("non_broadcast\n");break;  
					case  broadcast:   rt_kprintf("broadcast\n");  break;
				}	
				rt_memcpy(PHYPayload->mac_payload->node_id, &rx_message->payload[++offset+1], sizeof(PHYPayload->mac_payload->node_id));
	      //rt_memcpy(PHYPayload->mac_payload.node_id, &rx_message->payload[++offset+1], sizeof(PHYPayload->mac_payload.node_id));
				rt_kprintf("PHYPayload.mac_payload.node_id=");
				for(int i =0;i<sizeof(PHYPayload->mac_payload->node_id);i++)				
				{rt_kprintf("%02x",PHYPayload->mac_payload->node_id[i]);}
				rt_kprintf("\n");
        offset += sizeof(PHYPayload->mac_payload->node_id);			
	      rt_memcpy(PHYPayload->mac_payload->gateway_id, &rx_message->payload[offset], sizeof(PHYPayload->mac_payload->gateway_id));
				rt_kprintf("PHYPayload.mac_payload.gateway_id=");
				for(int i =0;i<sizeof(PHYPayload->mac_payload->gateway_id);i++)				
				{rt_kprintf("%02x",PHYPayload->mac_payload->gateway_id[i]);}
				rt_kprintf("\n");
        offset += sizeof(PHYPayload->mac_payload->gateway_id);	
				rt_memcpy(PHYPayload->mac_payload->event_id, &rx_message->payload[offset], sizeof(PHYPayload->mac_payload->event_id));
				rt_kprintf("PHYPayload->mac_payload.event_id= %0x\n",PHYPayload->mac_payload->event_id);
				offset += sizeof(PHYPayload->mac_payload->event_id);
				PHYPayload->mac_payload->fcmd = rx_message->payload[++offset];
				rt_kprintf("PHYPayload->mac_payload->message_id= %0x\n",PHYPayload->mac_payload->event_id);
				
        offset += sizeof(PHYPayload->mac_payload->message_id);
				rt_memcpy(PHYPayload->mac_payload->message_id, &rx_message->payload[offset], sizeof(PHYPayload->mac_payload->message_id));
				
				PHYPayload->mac_payload->flen = rx_message->payload[++offset]; 
				rt_memcpy(PHYPayload->mac_payload->frm_payload, &rx_message->payload[offset],mac_payload_length - offset);
     
			//	if(rx_message->payload[rx_message->payload_length-1]!=calculate_crc(&rx_message->payload[0],(rx_message->payload_length - 2)))
	    //return RT_ERROR;
		}
		//rt_mp_free(data);
   // rt_thread_mdelay(10);  // ??CPU??					
    return RT_EOK;
}
	

/**
 * @brief Encode PHYPayload_t into lora_tx_message_t;
 * @param PHYPayload The data structure to be encoded
 * @param tx_message Receive the encoded data
 * @return RT_EOK succeed, other value failed.
 * @program liujianfeng  2025/8/12
 */


int lora_encode(PHYPayload_t *PHYPayload, lora_message_t *tx_message) {

    if (PHYPayload == NULL || tx_message == NULL) {
        rt_kprintf("lora_encode: invalid param!\n");
        return -RT_EINVAL;
    }

		tx_message->payload = rt_malloc(tx_message->payload_length);
    if (tx_message->payload == NULL) {
        rt_kprintf("lora_encode: alloc payload failed!\n");
        return -RT_ENOMEM;
    }
    //uint16_t offset = 0;
		//tx_message->payload=&tx_message->payload[0];
    memset(tx_message->payload, 0, tx_message->payload_length); 
		
    tx_message->payload[0] = (PHYPayload->mhdr.mtype << 5)       //:mtype
                                | (PHYPayload->mhdr.upgrade << 4)    //:upgrade
                                | (PHYPayload->mhdr.broadcast << 3)  //:broadcast
                                | (PHYPayload->mhdr.protocol_version); //:protocol_version
     
    tx_message->payload[1]=PHYPayload->mlen;
		tx_message->payload[2]=PHYPayload->mac_payload->node_id[0];
		tx_message->payload[3]=PHYPayload->mac_payload->node_id[1];
		tx_message->payload[4]=PHYPayload->mac_payload->node_id[2];
		tx_message->payload[5]=PHYPayload->mac_payload->node_id[3];
		tx_message->payload[6]=PHYPayload->mac_payload->gateway_id[0];
		tx_message->payload[7]=PHYPayload->mac_payload->gateway_id[1];
	  tx_message->payload[8]=PHYPayload->mac_payload->gateway_id[2];
		tx_message->payload[9]=PHYPayload->mac_payload->gateway_id[3];
		tx_message->payload[10]=PHYPayload->mac_payload->event_id[0];
		tx_message->payload[11]=PHYPayload->mac_payload->event_id[1];
		tx_message->payload[12]= PHYPayload->mac_payload->message_id[0];
		tx_message->payload[13]= PHYPayload->mac_payload->message_id[1];
		tx_message->payload[14]= 0;//token
    tx_message->payload[15] = PHYPayload->mac_payload->fcmd;
    tx_message->payload[16] = PHYPayload->mac_payload->flen;
		if (PHYPayload->mac_payload->flen > 0 && PHYPayload->mac_payload->flen <= LORA_PAYLOAD_MAX_SIZE) {
				rt_memcpy(&tx_message->payload[17], PHYPayload->mac_payload->frm_payload, PHYPayload->mac_payload->flen);
		} else {
				//rt_kprintf("lora_encode: invalid frm_payload length!\n");
				//return -RT_EINVAL;
		}
		
		PHYPayload->crc = calculate_checksum(tx_message->payload, LORA_PAYLOAD_FIX_SIZE + PHYPayload->mac_payload->flen);
    tx_message->payload[tx_message->payload_length-1] = PHYPayload->crc;

		/*
		for(int i=0;i<tx_message->payload_length;i++)
		{
       rt_kprintf("tx_message->payload[%d]=%02x\n", i,tx_message->payload[i]);
		}*/
    rt_kprintf("lora_encode success! total length=%d\n", tx_message->payload_length);
    return RT_EOK;
}