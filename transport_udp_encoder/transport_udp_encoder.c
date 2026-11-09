#include "transport_udp_encoder.h"
#include "../transport_udp/transport_udp_message.h"
#include <stdint.h>
#include <rtthread.h>
#include "ec20_hal.h"
#include "../utils/utils.h"

int transport_udp_decode(raw_udp_message_t *raw_udp_message, transport_udp_message_t *transport_udp_message){
    if (raw_udp_message->payload_length < TRANSPORT_HEADER_FIX_LENGTH + TRANSPORT_DATA_FIX_LENGTH) {
        rt_kprintf("Error: message length is less than the min length of transport, length: %d \n", raw_udp_message->payload_length );
        return RT_EINVAL;
    }

		//2. transport_udp_header decode
    transport_udp_message->transport_udp_header->transport_data_encoding  = raw_udp_message->payload[0];
    transport_udp_message->transport_udp_header->transport_version        = raw_udp_message->payload[1];
    transport_udp_message->transport_udp_header->transport_data_length    = raw_udp_message->payload[2] | (raw_udp_message->payload[3] << 8);    
		//3. transport_udp_data decode
    // 3.1 gateway_id, node_id decode
    rt_memcpy( transport_udp_message->transport_udp_data->gateway_id,     raw_udp_message->payload + TRANSPORT_HEADER_FIX_LENGTH, GATEWAY_ID_LENGTH);
    rt_memcpy( transport_udp_message->transport_udp_data->node_id,        raw_udp_message->payload + TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH, NODE_ID_LENGTH);
		// 3.2 message_create_time decode
    rt_memcpy( transport_udp_message->transport_udp_data->message_create_time,   raw_udp_message->payload + TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH, CREATE_TIME_LENGTH);
		// 3.3 message_id decode
    transport_udp_message->transport_udp_data->message_id = (raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH]) |
                                                                 (raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 1] << 8) |
                                                                 (raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 2] << 16) |
                                                                 (raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 3] << 24);
		// 3.4 service_type-------service_data_encryption
    transport_udp_message->transport_udp_data->service_type                 = raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 4];
    transport_udp_message->transport_udp_data->service_version              = raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 5];
    transport_udp_message->transport_udp_data->service_data_encoding        = raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 6];
    transport_udp_message->transport_udp_data->service_data_protocol_version= raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 7];
    transport_udp_message->transport_udp_data->service_data_encryption      = raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 8];
		// 3.5 event_id decode
    transport_udp_message->transport_udp_data->event_id = (raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 9]) |
                                                            (raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 10] << 8) |
                                                             (raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 11] << 16) |
                                                             (raw_udp_message->payload[TRANSPORT_HEADER_FIX_LENGTH + GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH + 12] << 24);

    //4. service_data decode
    // 4.1 calculate business_data_payload length                                                       
    uint16_t business_data_length = transport_udp_message->transport_udp_header->transport_data_length - TRANSPORT_DATA_FIX_LENGTH;
    if (business_data_length + TRANSPORT_HEADER_FIX_LENGTH + TRANSPORT_DATA_FIX_LENGTH + 1 != raw_udp_message->payload_length) {
        rt_kprintf("Error: message length is not match, transport_data_length: %d, raw_udp_message_length: %d \n", 
            transport_udp_message->transport_udp_header->transport_data_length, raw_udp_message->payload_length);
        return RT_EINVAL;
    }
    // 4.2 business_data_payload decode
    transport_udp_message->service_data->business_data_payload_length = business_data_length;
    transport_udp_message->service_data->business_data_payload = (uint8_t*)rt_malloc(business_data_length);
    if (transport_udp_message->service_data->business_data_payload != RT_NULL) {
        rt_memcpy( transport_udp_message->service_data->business_data_payload, raw_udp_message->payload + TRANSPORT_HEADER_FIX_LENGTH + TRANSPORT_DATA_FIX_LENGTH, business_data_length);    
    } else {
        rt_kprintf("service_data->business_data_payload is NULL.\r\n");
    }

    //7. crc
    transport_udp_message->crc = raw_udp_message->payload[raw_udp_message->payload_length - 1];

		return RT_EOK;
}
int transport_udp_encode(transport_udp_message_t *transport_udp_message, raw_udp_message_t *raw_udp_message){
 
     //1. transport_udp_header    
     uint8_t index  = 0;
     raw_udp_message->payload[index++] 	= transport_udp_message->transport_udp_header->transport_data_encoding;
     raw_udp_message->payload [index++] = transport_udp_message->transport_udp_header->transport_version;
     raw_udp_message->payload [index++] = transport_udp_message->transport_udp_header->transport_data_length & 0xFF;
     raw_udp_message->payload [index++] = (transport_udp_message->transport_udp_header->transport_data_length >> 8) & 0xFF;

     //2. transport_udp_data : TRANSPORT_DATA_FIX_LENGTH == 41bytes
     rt_memcpy(raw_udp_message->payload + index, transport_udp_message->transport_udp_data->gateway_id,   GATEWAY_ID_LENGTH);
     rt_memcpy(raw_udp_message->payload + index + GATEWAY_ID_LENGTH, transport_udp_message->transport_udp_data->node_id,      NODE_ID_LENGTH);
     rt_memcpy(raw_udp_message->payload + index + GATEWAY_ID_LENGTH + NODE_ID_LENGTH, transport_udp_message->transport_udp_data->message_create_time, CREATE_TIME_LENGTH);
    
     //2.1 message_id
     index += GATEWAY_ID_LENGTH + NODE_ID_LENGTH + CREATE_TIME_LENGTH;
     raw_udp_message->payload[index++] = transport_udp_message->transport_udp_data->message_id & 0xFF;
	   raw_udp_message->payload[index++] = (transport_udp_message->transport_udp_data->message_id >> 8) & 0xFF;
	   raw_udp_message->payload[index++] = (transport_udp_message->transport_udp_data->message_id >> 16) & 0xFF;
	   raw_udp_message->payload[index++] = (transport_udp_message->transport_udp_data->message_id >> 24) & 0xFF;
	
     raw_udp_message->payload[index++] = transport_udp_message->transport_udp_data->service_type;
     raw_udp_message->payload[index++] = transport_udp_message->transport_udp_data->service_version;
     raw_udp_message->payload[index++] = transport_udp_message->transport_udp_data->service_data_encoding;
     raw_udp_message->payload[index++] = transport_udp_message->transport_udp_data->service_data_protocol_version;
     raw_udp_message->payload[index++] = transport_udp_message->transport_udp_data->service_data_encryption;

     //2.2 event_id
     raw_udp_message->payload[index++] = transport_udp_message->transport_udp_data->event_id & 0xFF;
	   raw_udp_message->payload[index++] = (transport_udp_message->transport_udp_data->event_id >> 8) & 0xFF;
	   raw_udp_message->payload[index++] = (transport_udp_message->transport_udp_data->event_id >> 16) & 0xFF;
	   raw_udp_message->payload[index++] = (transport_udp_message->transport_udp_data->event_id >> 24) & 0xFF;
     //3. service_data
     if(transport_udp_message->service_data->business_data_payload_length > 0 && transport_udp_message->service_data->business_data_payload != RT_NULL){
        rt_memcpy( &raw_udp_message->payload [index] ,transport_udp_message->service_data->business_data_payload, transport_udp_message->service_data->business_data_payload_length);
     } else if (transport_udp_message->service_data->business_data_payload_length > 0 && transport_udp_message->service_data->business_data_payload == RT_NULL) {
        rt_kprintf("Warning: business_data_payload is NULL but length > 0, service_type: %d\n", transport_udp_message->transport_udp_data->service_type);
     }
     index += transport_udp_message->service_data->business_data_payload_length;
      
     //4. crc
     uint8_t crc = calculate_checksum((rt_uint8_t*)raw_udp_message->payload, index);
     raw_udp_message->payload[index++] = crc;
     transport_udp_message->crc = crc;
}