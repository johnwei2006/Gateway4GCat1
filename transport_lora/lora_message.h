#ifndef LORA_MESSAGE_H_
#define LORA_MESSAGE_H_

#include <rtthread.h>
#include <stdint.h>

#define FCMD_NODE_PING                  0x67    //103
#define FCMD_NODE_PONG                  0x68    //104 
#define FCMD_NODE_INFO_QUERY            0x69    //105
#define FCMD_NODE_INFO_REPORT           0x6A    //106
#define FCMD_NODE_CONFIG_QUERY          0x6B    //107
#define FCMD_NODE_CONFIG_REPORT         0x6C    //108
#define FCMD_NODE_CONFIG                0x6D    //109
#define FCMD_NODE_CONFIG_RESPONSE       0x6E    //110
#define FCMD_NODE_SENSOR_DATA_UPLOAD_NOTICE_BATCH    0x6F    //111
#define FCMD_NODE_SENSOR_DATA_UPLOAD                 0x70    //112
#define FCMD_NODE_SENSOR_DATA_RE_UPLOAD_NOTICE_BATCH 0x71    //113
#define FCMD_SENSOR_CONFIG                0x73    //115
#define FCMD_SENSOR_CONFIG_RESPONSE       0x74    //116
#define FCMD_DEVICE_CONFIG_QUERY          0x79    //121
#define FCMD_DEVICE_CONFIG_RESPONSE       0x7A    //122      
#define FCMD_NODE_DEVICE_CONTROL          0x7B    //123  
#define FCMD_NODE_DEVICE_CONTROL_RESPONSE 0x7C    //124 
#define FCMD_NODE_DEVICE_CONTROL_BATCH    0x7D    //125   
#define FCMD_NODE_DEVICE_CONTROL_BATCH_RESPONSE    0x7E    //126    
#define FCMD_NODE_OTA_BATCH               0xBF    //191
#define FCMD_NODE_OTA_STATUS              0xC0    //192
#define FCMD_NODE_JOIN_RESPONSE           0xC5    //197
#define FCMD_NODE_JOIN_REQUEST            0xC6    //198
#define FCMD_NODE_REBOOT                  0xC7    //199
#define FCMD_NODE_MESSAGE_ACKNOWLEDGE     0xC8    //200

// version
#define PROTOCOL_VERSION (0x5) 

#define MAX_MESSAGE_ID     16384 // 最大计数差值
#define BROADCAST_NODEID   0x00000000 // 广播节点ID

// LORA_PAYLOAD: MHDR(1) + MLEN(1) + NodeID(4) + GatewayID(4) + MessageID(2) + SourceMessageID(2) + Token(1) + FCMD(1) + FLEN(1) + FRMPayload + CRC(1)	
#define LORA_PAYLOAD_FIX_SIZE    18 // PHYPayload min package length
#define LORA_PAYLOAD_MAX_SIZE    50 // PHYPayload max package length
#define LORA_FRMPAYLOAD_MAX_SIZE 32 // FRMPayload max length = LORA_PAYLOAD_MAX_SIZE - LORA_PAYLOAD_FIX_SIZE = 50 - 18 = 32
#define LORA_PAYLOAD_BLOCK_NUM   3  
#define LORA_MESSAGE_BLOCK_NUM   2

#pragma pack(4)

typedef enum{
  join_req  					= 0x01,
  join_apt 						= 0x02,
	unconfirmed_data_up = 0x03,
	unconfirmed_down_up = 0x04,
	confirmed_data_up 	= 0x05,
	confirmed_data_down = 0x06,
	ping_accept 				= 0x07,
} mtype;

typedef enum{
	LoRaWAN1_0x,
  LoRaWAN1_10,
} protocol_version_t;

typedef enum{
	upgarade_packet,
  ordinary_packet,
} upgarade_t;

typedef enum{
	non_broadcast,
  broadcast,
} broadcast_t;

typedef struct{
  uint8_t     *payload;
  uint8_t      payload_length;
  int16_t      rssi;
  int8_t       snr;
} lora_rx_message_t;

typedef struct{
  uint8_t     *payload;
  uint8_t     payload_length;
} lora_message_t;

typedef struct MHDR             //Mac Header 
{
   
   uint8_t    upgrade;
   uint8_t		broadcast;
	 uint8_t  	mtype; 
   uint8_t 		protocol_version;
} MHDR_t;

typedef struct MACPayload
{
   uint8_t    node_id[4];
   uint8_t    gateway_id[4];
   uint8_t    message_id[2];
   uint8_t    event_id[2];
	 uint8_t	  token;
   uint8_t    fcmd;
   uint8_t    flen;
   uint8_t    frm_payload[LORA_FRMPAYLOAD_MAX_SIZE];
   void       *service_data;
} MACPayload_t;       

typedef struct PHYPayload
{
   MHDR_t  		   mhdr;
   uint8_t 		   mlen;
   MACPayload_t   *mac_payload;
   uint8_t 		   crc;
} PHYPayload_t;   

#pragma pack()
#endif /*LORA_MESSAGE_H_*/
