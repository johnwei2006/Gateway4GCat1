#include "lora_handler.h"
#include "lora_encoder.h"
#include "lora_service_encoder.h"
#include <stddef.h>
#include "string.h"
#include <stdlib.h>
#include <rtthread.h>
#include "../transport/transport_message_queue.h"
#include "../transport/transport_mem_pool_manager.h"
#include "../config/config_manager.h"
#include "../service_manager/service_manager_lora.h"

#define THREAD_STACK_SIZE 1024
#define THREAD_PRIORITY  3
#define THREAD_TIMESLICE 10

rt_mutex_t lora_mem_mutex  = RT_NULL;
rt_sem_t lora_uplink_sem = RT_NULL;
rt_sem_t lora_downlink_sem = RT_NULL;

void lora_trigger_tx(void) {
    /*if (lora_tx_sem != RT_NULL) {
        rt_sem_release(lora_tx_sem);
    }*/
}
void lora_trigger_lora_uplink_sem(void) {
    if (lora_uplink_sem != RT_NULL) {
        rt_sem_release(lora_uplink_sem);
    }
}

void lora_downlink_process(void *parameter) {
    PHYPayload_t *PHYPayload = RT_NULL;
    lora_message_t lora_tx_message;

    while (1) {
			//1. get_node_downlink_message
			//rt_sem_take(lora_downlink_sem, RT_WAITING_FOREVER);
      PHYPayload = RT_NULL;
      rt_err_t ret = get_node_downlink_message(&PHYPayload);
      if (PHYPayload == RT_EOK) {
        rt_thread_mdelay(100); 
        continue;
      }
			rt_kprintf("********************************************************************\n");
      //rt_kprintf("lora_downlink_process: downlink message PHYPayload=%p, mac_payload=%p\n", PHYPayload, PHYPayload->mac_payload);
      
			//2. PHYPayload transport header init
			PHYPayload->mhdr.broadcast        = broadcast;
			PHYPayload->mhdr.mtype            = unconfirmed_down_up;
			PHYPayload->mhdr.protocol_version = LoRaWAN1_0x;
			PHYPayload->mhdr.upgrade          = ordinary_packet;

			config_gateway_serial_t gateway_config;
			read_config_gateway_serial(&gateway_config);
			memcpy(PHYPayload->mac_payload->gateway_id, gateway_config.gateway_id, 4);

      short message_id = generate_rand_short();
      PHYPayload->mac_payload->message_id[0] = message_id & 0xFF;
      PHYPayload->mac_payload->message_id[1] = (message_id >> 8) & 0xFF;
			//rt_kprintf("lora_downlink_process message_id : %d\r\n", message_id);
			
      rt_kprintf("lora_downlink_process: downlink message fcmd      : %d\n", PHYPayload->mac_payload->fcmd);      
      rt_kprintf("lora_downlink_process: downlink message event_id  : %#X\n", PHYPayload->mac_payload->event_id);
      rt_kprintf("lora_downlink_process: downlink message message_id: %#X\n", PHYPayload->mac_payload->message_id);
      //PHYPayload->mac_payload.flen = 10;
			//PHYPayload->crc= 0xAA;

			//3. Allocate memory for lora_tx_message
      //lora_message_mem_init(&lora_tx_message, PHYPayload);
			/*if (lora_tx_message == NULL) {
				rt_kprintf("lora_downlink_process: alloc lora_msg failed!\n");
				rt_thread_mdelay(100);
				continue;
			}*/

		// Total length: MHDR(1) + MLEN(1) + NodeID(4) + GatewayID(4) + MessageID(2) + SourceMessageID(2) + Token(1) + FCMD(1) + FLEN(1) + FRMPayload + CRC(1)	
      lora_tx_message.payload_length = PHYPayload->mac_payload->flen + LORA_PAYLOAD_FIX_SIZE;
      lora_tx_message.payload        = RT_NULL;
      
      //4. Encode PHYPayload to lora_tx_message
      ret = lora_encode(PHYPayload, &lora_tx_message);		
      if (ret != RT_EOK) {
        rt_kprintf("lora_downlink_process: encode failed!\n");
        release_mem_lora_payload(lora_tx_message.payload);
        release_mem_lora_message(&lora_tx_message);
				release_mem_PHYPayload(PHYPayload);
        rt_thread_mdelay(100);
        continue;
      } 
			
			//5. put_lora_downlink_message
      ret = put_lora_downlink_message(&lora_tx_message);
      if (ret != RT_EOK) {
				rt_kprintf("lora_downlink_process: put lora_msg failed!\n");
        rt_mp_free(lora_tx_message.payload);
        rt_mp_free(&lora_tx_message);
      }
      lora_trigger_tx();

      free(PHYPayload->mac_payload->service_data);
      PHYPayload->mac_payload->service_data = RT_NULL;
      free(PHYPayload->mac_payload->frm_payload);
      free(PHYPayload->mac_payload);
      PHYPayload->mac_payload = RT_NULL;
      free(PHYPayload);
      PHYPayload = RT_NULL; 
      //rt_mp_free(PHYPayload);
   }
}

void lora_uplink_process(void *parameter){
    lora_message_t *lora_rx_message = RT_NULL;
    PHYPayload_t *PHYPayload = RT_NULL;

		while (1) {
	 
        rt_sem_take(lora_uplink_sem, RT_WAITING_FOREVER);	    
				request_mem_PHYPayload(PHYPayload);   
        if (PHYPayload == NULL) {
            rt_kprintf("lora_uplink_process: alloc PHYPayload failed!\n");
            rt_mp_free(lora_rx_message);
            rt_thread_mdelay(100);
            continue;
        }			
	
        rt_memset(PHYPayload, 0, sizeof(PHYPayload_t));

        rt_err_t ret = get_lora_uplink_message(&lora_rx_message);
        if (ret != RT_EOK) {
            rt_mp_free(lora_rx_message);
            rt_mp_free(PHYPayload);
            rt_thread_mdelay(100);
            continue;
        }

        ret = lora_decode(lora_rx_message, PHYPayload);
        if (ret != RT_EOK) {
            rt_kprintf("lora_uplink_process: decode failed!\n");
            rt_mp_free(lora_rx_message);
            rt_mp_free(PHYPayload);
            rt_thread_mdelay(100);
            continue;
        }
        ret = lora_service_decode(PHYPayload);
        if (ret != RT_EOK) {
            rt_kprintf("lora_uplink_process: service decode failed!\n");
        }
				
        ret = service_manager_lora_process_uplink(PHYPayload);
        //ret = put_node_uplink_message(&PHYPayload);
        if (ret != RT_EOK) {
            rt_kprintf("lora_uplink_process: put node_uplink failed!\n");
            rt_mp_free(PHYPayload);
            PHYPayload = RT_NULL;
        }
				
        rt_mp_free(lora_rx_message);
        lora_rx_message = RT_NULL;
    }
}

int lora_handler_init(void){

		//1. init lora_uplink_thread
    if(lora_uplink_thread == RT_NULL){
			lora_uplink_thread = rt_thread_create("lora_uplink_thread", 
                                             lora_uplink_process, 
                                             RT_NULL, 
                                             1536, 
                                             10, 
                                             25);
    if (lora_uplink_thread != RT_NULL) {
				rt_err_t ret = rt_thread_startup(lora_uplink_thread);                            
				if (ret != RT_EOK) {
						rt_kprintf("failed to start lora_uplink_thread \n"); 
						rt_thread_delete(lora_uplink_thread); 
						lora_uplink_thread = RT_NULL;
						return RT_ERROR;  
				}
			}	
		}

		//2. init lora_downlink_thread
		if (lora_downlink_thread == RT_NULL){
				lora_downlink_thread = rt_thread_create("lora_downlink_thread", 
                                               lora_downlink_process, 
                                               RT_NULL, 
                                               1536, 
                                               10, 
                                               25);
		if (lora_downlink_thread != RT_NULL) {   
			rt_err_t ret = rt_thread_startup(lora_downlink_thread);
      if (ret != RT_EOK) {
				rt_kprintf("failed to start lora_downlink_thread \n"); 
        rt_thread_delete(lora_downlink_thread); 
        lora_downlink_thread = RT_NULL;
        return RT_ERROR; 
      } 			
		}	

		/*3. lora_mem_mutex*/
	   if (lora_mem_mutex == RT_NULL) {
        lora_mem_mutex = rt_mutex_create("lora_mem_mutex", RT_IPC_FLAG_FIFO);
        if (lora_mem_mutex == RT_NULL) {
            rt_kprintf("Failed to create lora mem mutex!\n");
            return -1;
        }
      }

	  //4. lora_uplink_sem
		 if (lora_uplink_sem == RT_NULL) {
        lora_uplink_sem = rt_sem_create("lora_up_sem", 0, RT_IPC_FLAG_FIFO);
        if (lora_uplink_sem == RT_NULL) {
            rt_kprintf("Failed to create lora_uplink_sem!\n");
            return -1;
        }
    }

		//5. lora_downlink_sem
		 if (lora_downlink_sem == RT_NULL) {
        lora_downlink_sem = rt_sem_create("lora_down_sem", 0, RT_IPC_FLAG_FIFO);
        if (lora_downlink_sem == RT_NULL) {
            rt_kprintf("Failed to create lora_downlink_sem!\n");
            return -1;
        }
    }
    return RT_EOK;
	}
}

void lora_message_mem_init(lora_message_t *lora_tx_message, PHYPayload_t *PHYPayload){
    //rt_mutex_take(lora_mem_mutex, RT_WAITING_FOREVER);
		request_mem_lora_message(&lora_tx_message);
    //rt_mutex_release(lora_mem_mutex);
        
    if (lora_tx_message == NULL) {
			rt_kprintf("lora_downlink_process: alloc lora_msg failed!\n");
      release_mem_lora_message(lora_tx_message);  
      rt_thread_mdelay(100);
		}
   
		rt_memset(lora_tx_message, 0, sizeof(lora_message_t));
         
		request_mem_lora_payload(&lora_tx_message->payload);
    if (lora_tx_message->payload == NULL) {
				rt_kprintf("lora_downlink_process: alloc payload failed!\n");
        release_mem_lora_message(lora_tx_message);  
				release_mem_PHYPayload(PHYPayload);
        rt_thread_mdelay(100);
		}
        
    rt_memset(lora_tx_message->payload, 0, LORA_PAYLOAD_MAX_SIZE);
}	

//INIT_ENV_EXPORT(lora_handler_init);



