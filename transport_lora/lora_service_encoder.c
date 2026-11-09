#include "lora_service_encoder.h"
#include "../transport_lora/lora_message.h"
#include <stdio.h>
#include <string.h>
#include "utils.h"

// convert PHYPayload->mac_payload->service_data to PHYPayload.mac_payload->frm_payload
int lora_service_encode(PHYPayload_t *PHYPayload){
    if (PHYPayload == NULL ) {
        rt_kprintf("lora_service_encode: invalid params PHYPayload!\n");
        return -RT_EINVAL;
    }

    uint8_t flen = PHYPayload->mac_payload->flen;
    uint8_t fcmd = PHYPayload->mac_payload->fcmd;
    if(fcmd == FCMD_NODE_PONG) {
        memcpy(PHYPayload->mac_payload->frm_payload, PHYPayload->mac_payload->service_data, flen);
    } else if(fcmd == FCMD_NODE_SENSOR_DATA_UPLOAD_NOTICE_BATCH) {
        //memcpy(business_data, PHYPayload->mac_payload->service_data,PHYPayload->mac_payload->flen);
        rt_kprintf("lora_service_encode: copied %d bytes of business data\n", PHYPayload->mac_payload->flen);
    } else {
        rt_kprintf("lora_service_encode: unsupported fcmd 0x%02x\n", PHYPayload->mac_payload->fcmd);
        return -RT_EINVAL;
    }

    return RT_EOK;
}

 // conert PHYPayload->mac_payload.frm_payload to PHYPayload->mac_payload.service_data;
int lora_service_decode(PHYPayload_t *PHYPayload){
    if (PHYPayload == NULL ) {
        rt_kprintf("lora_service_decode: invalid params PHYPayload!\n");
        return -RT_EINVAL;
    }

    uint8_t flen = PHYPayload->mac_payload->flen;
    uint8_t fcmd = PHYPayload->mac_payload->fcmd;
    if(fcmd == FCMD_NODE_PING){
        rt_kprintf("lora_service_decode: FCMD_NODE_PING, flen=%d, pong=%s\n", flen, PHYPayload->mac_payload->frm_payload);
        PHYPayload->mac_payload->service_data = PHYPayload->mac_payload->frm_payload;
    } else if(fcmd == FCMD_NODE_JOIN_REQUEST){
       	/*				
        NodeJoinRequest.firm_type      = PHYPayload->mac_payload->frm_payload[0];
			  memcpy(NodeJoinRequest.firm_version,&PHYPayload->mac_payload->frm_payload[1] , 4);
			  
        rt_kprintf("NodeJoinRequest.firm_version");
        for(int i=1;i<5;i++)
        {
					rt_kprintf("%02x",PHYPayload->mac_payload->frm_payload[i]);
				}
				rt_kprintf("\n");			     
 
			  memcpy(NodeJoinRequest.uboot_version,&PHYPayload->mac_payload->frm_payload[5] , 4);		
			  				
				rt_kprintf("NodeJoinRequest.uboot_version");
        for(int i=0;i<4;i++)
        {
					rt_kprintf("%02x",NodeJoinRequest.uboot_version[i]);
				}
				rt_kprintf("\n");												
				
			  NodeJoinRequest.freq_group   =  PHYPayload->mac_payload->frm_payload[9];
				rt_kprintf("NodeJoinRequest.freq_group=");
				rt_kprintf("%02x",NodeJoinRequest.freq_group);
				rt_kprintf("\n");	
        //business_data = &NodeJoinRequest;
			  *business_data = &NodeJoinRequest;
        PHYPayload->mac_payload->service_data = &NodeJoinRequest;
    } else if (fcmd == FCMD_NODE_MESSAGE_ACKNOWLEDGE){
        uint8_t size = flen/4;
        for(uint8_t i = 0; i < size; i++){
            memcpy(ackDetail.node_seqence, PHYPayload->mac_payload->frm_payload + 2*i, 2);
					  rt_kprintf("ackDetail.node_seqence=");
						for(int i=0;i<2;i++)
						{
							rt_kprintf("%02x",ackDetail.node_seqence[i]);
						}
				rt_kprintf("\n");			
            memcpy(ackDetail.message_id, PHYPayload->mac_payload->frm_payload + 2*(i+1), 2);
						 rt_kprintf("ackDetail.message_id=");
						for(int i=0;i<2;i++)
						{
							rt_kprintf("%02x",ackDetail.message_id[i]);
						}
				   rt_kprintf("\n");	
            acknowledge.node_message_acknowledge_detail[i] = ackDetail;						
						rt_kprintf("ackDetail.message_id=%02x",acknowledge.node_message_acknowledge_detail[i].node_seqence[0],acknowledge.node_message_acknowledge_detail[i].node_seqence[1]);
						rt_kprintf("message_id=%02x",acknowledge.node_message_acknowledge_detail[i].message_id[0],acknowledge.node_message_acknowledge_detail[i].message_id[1]);
						
        }
				 *business_data = &NodeJoinRequest;
        //business_data = &acknowledge;
         PHYPayload->mac_payload->service_data = &acknowledge;
         */
    }
}

void lora_service_encoder_init(){
    return;
}
