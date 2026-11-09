#ifndef __CONFIG_MANAGER_H
#define __CONFIG_MANAGER_H
#include <rtdef.h>
#include <stdint.h>
#include <lora-radio.h>
#include "../utils/cps_list.h"
#include "../flash_external/abstract_flash_external.h"


#define PROJECT_ID_MAX_LENGTH    11
#define GATEWAY_ID_LENGTH        11
#define NODE_ID_LENGTH           11
#define SERVER_URL_MAX_LENGTH    40
#define FIRMWARE_VERSION_LENGTH  3
#define DEVICENAME_MAX_LENGTH    20
#define DEVICE_CODE_MAX_LENGTH  13 // 设备编码最大长度 + node_id + serial_number(2 bytes)
#define COMMAND_HEX_MAX_LENGTH  26 // 485命令最大长度，需根据实际情况调整，包含CRC校验位
#define MAX_NODES               15  // 最大节点数量
#define MAX_CHANNELS            16  // 最大通道数量
#define GATEWAY_CONFIG_BLOCK_NUM  0   // 网关配置块号
#define MAX_TASKS_COUNT         5  // 最大任务数量

static uint8_t HEARDWARE_VERSION[FIRMWARE_VERSION_LENGTH] = {0x1A, 0x01, 0x0F}; //260115 
static uint8_t FIRMWARE_VERSION[FIRMWARE_VERSION_LENGTH]  = {0x1A, 0x06, 0x1D}; //260629 

#define THREAD_LORA_PRIORITY 						3
#define THREAD_LORA_TIMESLICE 					20
#define THREAD_LORA_STACK_SIZE 					1024 * 2
#define DEFAULT_GATEWAY_TASK_ID         "task_id0001"

#pragma pack(1)
typedef enum {
  NODE_TYPE_DATA    = 1,
  NODE_TYPE_IRRI    = 2,
  NODE_TYPE_ACT     = 3,
  NODE_TYPE_MOTOR   = 4,
  NODE_TYPE_VIRTURAL= 9
} node_type_t;

typedef enum {
  MODBUS_COMMAND_TYPE_READ    = 1,
  MODBUS_COMMAND_TYPE_ON      = 2,
  MODBUS_COMMAND_TYPE_OFF     = 3,
  MODBUS_COMMAND_TYPE_STOP    = 4,
  MODBUS_COMMAND_TYPE_SET     = 5 
} modbus_command_type_t;

typedef enum {
  TRIGGER_TYPE_INTERVAL = 1,
  TRIGGER_TYPE_CONDITION = 2,
  TRIGGER_TYPE_MANUAL = 3
} task_trigger_type_t;

typedef enum {
  TRIGGER_JOIN_TYPE_AND = 1,
  TRIGGER_JOIN_TYPE_OR = 2
} task_trigger_join_type_t;

typedef enum {
  TRIGGER_COMPARE_TYPE_EQUAL = 1,
  TRIGGER_COMPARE_TYPE_NOT_EQUAL = 2,
  TRIGGER_COMPARE_TYPE_GREATER = 3,
  TRIGGER_COMPARE_TYPE_LESS = 4,
  TRIGGER_COMPARE_TYPE_GREATER_EQUAL = 5,
  TRIGGER_COMPARE_TYPE_LESS_EQUAL = 6,
} task_trigger_compare_type_t;

typedef enum {
  ACTION_TYPE_ON = 1,
  ACTION_TYPE_OFF = 2,
  ACTION_TYPE_SET = 3,
} task_action_type_t;

typedef struct{
  uint8_t           loraFreqGroup;
  //uint8_t           loraSF;
  //uint8_t           loraBW;
  //uint8_t           loraCR;
  uint8_t           loraTxPower;
} config_lora_t;

typedef struct
{
    //uint8_t  enabled;       // 是否启用
    uint8_t  channel;         // sensor channel(logic:defined in platform)
    //uint8_t  sensor_class;	  // sensor class    
    uint16_t sensor_type;	    // sensor type
    uint8_t  sensor_code[DEVICE_CODE_MAX_LENGTH];	      // sensor code
    uint8_t  read_hex[COMMAND_HEX_MAX_LENGTH];  // 485 read sensor value command(hex)--virtual node
    //uint8_t  modbus_address;			                      // 485 sensor address                --physical node    
} config_sensor_t;/*传感器数据结构 : 1 + 1 + 1 + 13 + 26 + 1 = 42 bytes*/

typedef struct
{
    uint8_t         node_id[NODE_ID_LENGTH];    
    uint8_t         sensor_count;    
    config_sensor_t sensors[MAX_CHANNELS];	
    uint8_t         config_time[6]; 
} config_sensor_list_t;

typedef struct {
    uint8_t  channel;// device channel(logic:defined in platform)  
    uint8_t  switch_type;    // switch type: 1-开度, 2-两开 3-三开
    //uint8_t  device_class;	 // device class        
    uint16_t device_type;   // device type
    uint8_t  device_code[DEVICE_CODE_MAX_LENGTH];
    uint8_t  on_hex[COMMAND_HEX_MAX_LENGTH];			// 485 on devicecommand(hex)   
    uint8_t  off_hex[COMMAND_HEX_MAX_LENGTH];			// 485 off devicecommand(hex)    
    uint8_t  set_hex[COMMAND_HEX_MAX_LENGTH];			// 485 stop or set device command(hex)  switch_type=1 using set command, switch_type=3 using stop command              
} config_device_t; /*设备数据结构 : channel + switch_type + device_class + device_type + device_code + on_command + off_command + stop_or_set_command = 1 + 1 + 1 + 1 + 13 + 26 + 26 + 26 = 95 bytes*/

typedef struct
{
    uint8_t         node_id[NODE_ID_LENGTH];    
    uint8_t         device_count;    
    config_device_t devices[MAX_CHANNELS];	
    uint8_t         config_time[6]; 
} config_device_list_t;

typedef struct {
   node_type_t      node_type;
   uint8_t          node_id[NODE_ID_LENGTH];
   uint16_t         report_interval;
  // config_lora_t    config_lora_node;
  cps_list_node_t   *config_sensors;
  cps_list_node_t   *config_devices;
  uint8_t            config_time[6];  
} config_node_t;

typedef struct {
   node_type_t      node_type;
   uint8_t          node_id[NODE_ID_LENGTH]; 
   uint16_t         report_interval;
  uint8_t           sensors_count;
  uint8_t           sensor_block_numbers[MAX_CHANNELS];
  uint8_t           devices_count;
   uint8_t          device_block_numbers[MAX_CHANNELS];
   uint8_t          config_time[6];  
   uint8_t          lora_freq_group;
} config_node_serial_t; // 用于传输的节点序列化配置结构体: 1 + 12 + 2 + 1 + 16 + 1 + 16 + 6 = 55 bytes

typedef struct {
   uint8_t              nodes_count;
   config_node_serial_t config_nodes[MAX_NODES];
   uint8_t          config_time[6];  
} config_node_list_t; 

typedef struct {
  uint8_t            config_version;
  uint8_t            project_id[PROJECT_ID_MAX_LENGTH];
  uint8_t            gateway_id[GATEWAY_ID_LENGTH];
  uint8_t            system_server_url[SERVER_URL_MAX_LENGTH];
  //uint8_t            business_server_url[SERVER_URL_MAX_LENGTH];
  uint16_t           system_server_port;
  uint16_t           report_interval;
  uint16_t           heartbeat_interval;
  config_lora_t      config_lora;
  cps_list_node_t   *config_nodes;
  uint8_t            config_time[6];
} config_gateway_t;

typedef struct {
  uint8_t            config_version;
  uint8_t            project_id[PROJECT_ID_MAX_LENGTH + 1]; // +1 for null terminator
  uint8_t            gateway_id[GATEWAY_ID_LENGTH + 1];
  uint8_t            system_server_url[SERVER_URL_MAX_LENGTH + 1]; // +1 for null terminator
  //uint8_t            business_server_url[SERVER_URL_MAX_LENGTH + 1]; // +1 for null terminator
  uint16_t           system_server_port;
  uint16_t           heartbeat_interval;
  uint16_t           report_interval;
  config_lora_t      config_lora;
  uint8_t            nodes_count;
  uint8_t            node_block_numbers[MAX_NODES];
  uint8_t            config_time[6];  
} config_gateway_serial_t; // 用于传输的网关序列化配置结构体: 1+ 12 + 12 + 40 + 2 + 2 + 2 + 1 + 15 + 6= 93 bytes

typedef struct {
  uint8_t            join_type;
  uint8_t            node_id[NODE_ID_LENGTH];
  uint8_t            channel;
  uint8_t            compare_type;
  uint16_t           value;
} config_gateway_task_trigger_t; // 任务触发条件结构体: 1 + 11 + 1 + 1 + 2 = 16 bytes

typedef struct {
  uint8_t            node_id[NODE_ID_LENGTH];
  uint8_t            channel;
  uint8_t            action;
  uint16_t           value;
} config_gateway_task_action_t; // 任务执行动作结构体:  11 + 1 + 1 + 2 = 15 bytes

typedef struct {
  uint8_t            task_id;
  uint8_t            enabled;
  uint8_t            trigger_type;
  uint8_t            trigger_time[3];
  uint8_t            trigger_size;
  config_gateway_task_trigger_t triggers[2];
  uint8_t            action_size;
  config_gateway_task_action_t  actions[3];
} config_gateway_task_t; // 任务结构体: 3 + 1 + 1 + 3 + 16*2 + 15*3 + 2 = 87 bytes

typedef struct
{ 
    uint8_t  task_count;    
    config_gateway_task_t config_tasks[MAX_TASKS_COUNT];	
} config_gateway_task_list_t;

typedef struct {
    RadioModems_t modem;        
	  uint32_t frequency;
    uint32_t fdev;             
    uint32_t bandwidth;    
		uint32_t datarate;
		uint16_t preambleLen;
		int8_t power;         
    uint8_t coderate;          
    bool fixLen;           
    bool crcOn;               
    bool freqHopOn;          
    uint8_t hopPeriod;        
    bool iqInverted;          
    uint32_t timeout;       
} loratxparams_t;					

typedef struct{
	RadioModems_t modem;
	uint32_t frequency;
	uint32_t bandwidth;
	uint32_t datarate;
	uint32_t bandwidthAfc;
	uint16_t preambleLen;
	uint8_t coderate;
  uint16_t symbTimeout;
	uint8_t payloadLen;
	uint8_t hopPeriod;	
	rt_bool_t fixLen;
	rt_bool_t crcOn;
	rt_bool_t freqHopOn;	
	rt_bool_t iqInverted;
	rt_bool_t rxContinuous;
} lorarxparams_t;

#pragma pack()

rt_bool_t config_init(void);

/**
 * @brief 配置写入（适配#ST命令，JSON串→结构体→Flash）
 * @param key 配置Key（如GATEWAY_CONFIG）
 * @param json_value 外部传入的完整JSON串
 * @return 成功返回true，失败返回false
 */
bool config_manager_write_config(char* key, const char* json_value);

/**
 * @brief 配置读取（适配#RD命令，Flash→结构体→JSON串）
 * @param key 配置Key（支持CONFIG_SENSOR.NODE0000001）
 * @param out_json_buf 输出JSON缓冲区（需上层提前分配）
 * @param buf_len 缓冲区长度
 * @return 成功返回true，失败返回false
 */
bool config_manager_read_config(char* key, char* out_json_buf, uint32_t buf_len);

// Gateway config read/write from flash
flash_status read_config_gateway_serial(config_gateway_serial_t *gateway_config);
static flash_status write_config_gateway_serial(config_gateway_serial_t *gateway_config);
flash_status update_config_gateway(config_gateway_serial_t *gateway_config);

// Node config read/write from flash
flash_status read_config_node_serial(config_node_serial_t *config_node, const char* node_id);
static flash_status add_or_update_config_node_serial(config_node_serial_t *config_node, uint16_t *block_num);
flash_status update_config_node(config_node_list_t *config_node_list);
flash_status query_config_node_list(config_node_list_t *config_node_list);

// Full Node config read/delete from flash
static flash_status delete_config_node_all_sensor_device(const char* node_id);

// Sensor config read/write/delete from flash
static flash_status read_config_sensor(config_sensor_t *config_sensor, const char* node_id, uint16_t block_num);
flash_status query_config_sensor_by_channel(config_sensor_t *config_sensor, const char* node_id, uint8_t channel);
static flash_status add_or_update_config_sensor(config_sensor_t *config_sensor, const char* node_id, uint16_t *block_num);
static flash_status delete_config_sensor(config_sensor_t *config_sensor, const char* node_id);
flash_status update_config_sensor_list(config_sensor_list_t *config_sensor_list);
flash_status query_config_sensor_list(config_sensor_list_t *config_sensor_list);

// Device config read/write/delete from flash
flash_status read_config_device(config_device_t *config_device, const char* node_id, uint16_t block_num);
flash_status query_config_device_by_channel(config_device_t *config_device, const char* node_id, uint8_t channel);
static flash_status add_or_update_config_device(config_device_t *config_device, const char* node_id, uint16_t *block_num);
static flash_status delete_config_device(config_device_t *config_device, const char* node_id);
flash_status update_config_device_list(config_device_list_t *config_device_list);
flash_status query_config_device_list(config_device_list_t *config_device_list);

flash_status add_or_update_config_task_list(config_gateway_task_list_t *config_task_list);
flash_status query_config_task_list(config_gateway_task_list_t *config_task_list);
flash_status delete_config_task(config_gateway_task_t *config_task);
flash_status add_or_update_config_task(config_gateway_task_t *config_task);
#endif /* _CONFIG_MANAGER_H */
