#ifndef TRANSPORT_UDP_MESSAGE_H_
#define TRANSPORT_UDP_MESSAGE_H_

#include <stdint.h>

#define SERVICE_TYPE_ACK                                    0x00    //0
#define SERVICE_TYPE_HEARTBEAT                              0x01    //1 
#define SERVICE_TYPE_TIME_SYNC_RESPONSE                     0x03    //3
#define SERVICE_TYPE_TIME_SYNC_REQUEST                      0x04    //4 
#define SERVICE_TYPE_GATEWAY_INFO_QUERY                     0x05    //5 
#define SERVICE_TYPE_GATEWAY_INFO_REPORT                    0x06    //6  
#define SERVICE_TYPE_GATEWAY_CONFIG_QUERY                   0x07    //7
#define SERVICE_TYPE_GATEWAY_CONFIG_REPORT                  0x08    //8 
#define SERVICE_TYPE_GATEWAY_CONFIG                         0x09    //9
#define SERVICE_TYPE_GATEWAY_CONFIG_RESPONSE                0x0A    //10
#define SERVICE_TYPE_GATEWAY_CONFIG_NODE_QUERY              0x0B    //11
#define SERVICE_TYPE_GATEWAY_CONFIG_NODE_REPORT             0x0C    //12
#define SERVICE_TYPE_GATEWAY_CONFIG_NODE                    0x0D    //13
#define SERVICE_TYPE_GATEWAY_CONFIG_NODE_RESPONSE           0x0E    //14
#define SERVICE_TYPE_GATEWAY_CONFIG_SENSOR_QUERY            0x0F    //15
#define SERVICE_TYPE_GATEWAY_CONFIG_SENSOR_REPORT           0x10    //16
#define SERVICE_TYPE_GATEWAY_CONFIG_SENSOR                  0x11    //17
#define SERVICE_TYPE_GATEWAY_CONFIG_SENSOR_RESPONSE         0x12    //18
#define SERVICE_TYPE_GATEWAY_CONFIG_DEVICE_QUERY            0x13    //19
#define SERVICE_TYPE_GATEWAY_CONFIG_DEVICE_REPORT           0x14    //20
#define SERVICE_TYPE_GATEWAY_CONFIG_DEVICE                  0x15    //21
#define SERVICE_TYPE_GATEWAY_CONFIG_DEVICE_RESPONSE         0x16    //22
#define SERVICE_TYPE_GATEWAY_SENSOR_DATA_UPLOAD_NOTICE      0x17    //23
#define SERVICE_TYPE_GATEWAY_SENSOR_DATA_REPORT             0x18    //24
#define SERVICE_TYPE_GATEWAY_DEVICE_CONTROL                 0x19    //25
#define SERVICE_TYPE_GATEWAY_DEVICE_CONTROL_RESPONSE        0x1A    //26
#define SERVICE_TYPE_GATEWAY_DEVICE_CONTROL_BATCH           0x1B    //27
#define SERVICE_TYPE_GATEWAY_DEVICE_CONTROL_BATCH_RESPONSE  0x1C    //28
#define SERVICE_TYPE_GATEWAY_CONFIG_TASK_QUERY              0x21    //33 downlink: query gateway config task
#define SERVICE_TYPE_GATEWAY_CONFIG_TASK_REPORT             0x22    //34 uplink  : report gateway config task
#define SERVICE_TYPE_GATEWAY_CONFIG_TASK                    0x23    //35 uplink  : response gateway config task
#define SERVICE_TYPE_GATEWAY_CONFIG_TASK_RESPONSE           0x24    //36 uplink  : response gateway config task
#define SERVICE_TYPE_GATEWAY_CONFIG_TASK_LIST               0x25    //37 uplink  : response gateway config task list
#define SERVICE_TYPE_GATEWAY_CONFIG_TASK_LIST_RESPONSE      0x26    //38 uplink  : response gateway config task
#define SERVICE_TYPE_GATEWAY_UPGRADE                        0x5C    //91 downlink: send & start upgrade task
#define SERVICE_TYPE_GATEWAY_UPGRADE_STATUS                 0x5D    //92 uplink  : report upgrade status
#define SERVICE_TYPE_GATEWAY_GNSS                           0x60    //96 uplink  : report GNSS status
#define SERVICE_TYPE_GATEWAY_REBOOT                         0x65    //99 downlink: send reboot command   


#define SERVICE_TYPE_NODE_JOIN_REQUEST                      0xC6    //198 downlink: node join requestresponse

#define TRANSPORT_DATA_ENCODING_JSON     1
#define TRANSPORT_DATA_ENCODING_BINARY   2
#define TRANSPORT_VERSION_1              1
#define SERVICE_VERSION_1                1
#define SERVICE_DATA_ENCODING_JSON       1
#define SERVICE_DATA_ENCODING_BINARY     2
#define SERVICE_DATA_PROTOCOL_VERSION_1  1
#define SERVICE_DATA_ENCRYPTION_NONE     0
#define SERVICE_DATA_ENCRYPTION_AES128   1
//fix length wittout business data： transport_data_encoding(1) + transport_version(1) + transport_data_length(2)
#define TRANSPORT_HEADER_FIX_LENGTH      4     
//fix length wittout business data： gateway_id(11) + node_id(11) + message_create_time(6) + message_id(4) + service_type(1) + service_version(1) + service_data_encoding(1) + service_data_protocol_version(1) + service_data_encryption(1) + event_id(4)
#define TRANSPORT_DATA_FIX_LENGTH        41     
#define GATEWAY_ID_LENGTH                11
#define NODE_ID_LENGTH                   11
#define CREATE_TIME_LENGTH               6
#define MESSAGE_ID_LENGTH                4
#define MAX_RESPONSE_MESSAGE_LENGTH      30

#define DEVICE_CONTROL_ITEM_LENGTH       6  //channel(1) + action(1) + value(4)
#define DEVICE_CONTROL_ACTION_ON         1
#define DEVICE_CONTROL_ACTION_OFF        2
#define DEVICE_CONTROL_ACTION_STOP       3
#define DEVICE_CONTROL_ACTION_SET        4

typedef struct{
    char        *payload;
    uint16_t     payload_length;   
} raw_udp_message_t;

typedef struct
{
    uint8_t     transport_data_encoding;
    uint8_t     transport_version;
    uint16_t    transport_data_length;
} transport_udp_header_t;

typedef struct
{
	  uint32_t    message_id;
    uint32_t    event_id;        
    uint8_t     gateway_id[11];
    uint8_t     node_id[11];    
    uint8_t     message_create_time[6];
    uint8_t     service_type;
    uint8_t     service_version;
    uint8_t     service_data_encoding;
    uint8_t     service_data_protocol_version;
    uint8_t     service_data_encryption;
} transport_udp_data_t;

typedef struct
{
    uint8_t           *business_data_payload;
    uint8_t           business_data_payload_length;  
    void              *business_data; 
} service_data_t;

typedef struct
{
    transport_udp_header_t 	*transport_udp_header;
    transport_udp_data_t    *transport_udp_data;
    service_data_t          *service_data;
    uint8_t                  crc;
} transport_udp_message_t;

#endif /*TRANSPORT_UDP_MESSAGE_H_*/
