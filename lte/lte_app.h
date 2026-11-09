#ifndef __LTE_APP_H
#define __LTE_APP_H

#include <rtthread.h>
#include "ec20_protocol_udp.h"

#define RAW_UDP_THREAD_STACK_SIZE        1024
#define RAW_UDP_THREAD_PRIORITY          10
#define RAW_UDP_THREAD_TIMESLICE         20

// UDP应用层状态枚举
typedef enum {
    UDP_STATE_UNINIT = 0,    // 未初始化
    UDP_STATE_INITED,        // 已初始化
    UDP_STATE_CONNECTING,    // 连接中
    UDP_STATE_CONNECTED,     // 已连接
    UDP_STATE_DISCONNECTED,  // 已断开
    UDP_STATE_ERROR          // 错误状态
} lte_state_t;

// MQTT应用层状态枚举
typedef enum {
    MQTT_STATE_UNINIT = 0,    // 未初始化
    MQTT_STATE_INITED,        // 已初始化
    MQTT_STATE_CONNECTING,    // 连接中
    MQTT_STATE_CONNECTED,     // 已连接
    MQTT_STATE_DISCONNECTED,  // 已断开
    MQTT_STATE_ERROR          // 错误状态
} mqtt_state_t;

typedef struct {
    lte_state_t     lte_state;           // 连接状态
    char            server_ip[32];       // 服务器IP
    uint16_t        server_port;         // 服务器端口
    uint32_t        last_error;          // 最后错误码
    uint8_t         ec20_csq;            // 信号强度
    char *          ec20_imei;           // 模块IMEI
    char*           ec20_iccid;          // 模块ICCID
	char*			ec20_imsi;					//IMSI
    char*			ec20_operator;				//运营商
    char*           ec20_gmi;           // 模块GMI
    char*           ec20_gmm;           // 模块GMM
    char*           ec20_gmr;           // 模块GMR
    lte_time_t      lte_time;            // 模块时间
} lte_info_t;

typedef struct
{
  void (*LTE_SetCommandsFirst)(at_response_t resp); //获取第一个命令
  at_command_process_t **(*LTE_GetCommands)(void);  //获取命令组
} lte_commands_port_t;

typedef struct
{
  at_response_t (*LTE_Boot)(void); //LTE启动
  void (*LTE_Reboot)(void); //LTE复位
  lte_commands_port_t netCommands;  //网络命令接口
  lte_commands_port_t udp_connect_commands; //UDP连接命令接口
  lte_commands_port_t udp_send_data_commands; //UDP发送数据命令接口
  lte_commands_port_t check_connect_commands; //P连接检查命令接口
  lte_commands_port_t httpCommands; //Http命令接口
} lte_port_t;

extern rt_thread_t uplink_message_send_thread;

extern lte_port_t ltePort;

// 业务数据回调类型（传递解析后的有效数据）
typedef void (*udp_business_callback)(const char *data, uint16_t len);

// 初始化UDP应用
int udp_app_init(const char *server_ip, uint16_t server_port);

void get_lte_info(lte_info_t* lte_info);

void set_lte_status(lte_state_t lte_state);
// 启动UDP连接
int udp_app_connect(rt_bool_t is_reconnect, void (*connect_callback)(lte_state_t lte_status));

// 强制重连UDP服务器
int udp_app_reconnect(void);

// 发送业务数据（自动封装格式）
int udp_app_send(const char *data, uint16_t len);

// 释放UDP资源
void udp_app_deinit(void);

// MQTT应用相关函数声明
int mqtt_app_init(const char *server_ip, uint16_t server_port, const char *client_id, const char *username, const char *password);
int mqtt_app_connect(rt_bool_t is_reconnect);
int mqtt_app_reconnect(void);
int mqtt_app_publish(const char *topic, const char *data, uint16_t len, uint8_t qos);
int mqtt_app_subscribe(const char *topic, uint8_t qos);
int mqtt_app_unsubscribe(const char *topic);
void mqtt_app_deinit(void);

static void udp_app_uplink_process(void* data);

static void mqtt_app_uplink_process(void* data);

extern int lte_command_proccess(at_command_process_t **pCommands);

static void urc_mqtt_status_handler(struct at_client *client, const char *data, rt_size_t size);

static void urc_mqtt_recv_handler(struct at_client *client, const char *data, rt_size_t size);

static void urc_send_data_handler(struct at_client *client, const char *data, rt_size_t size);

static void urc_recv_data_handler(struct at_client *client, char *data, rt_size_t size);

static void urc_rd_data_handler(struct at_client *client, char *data, rt_size_t len);

static void urc_closed_handler(struct at_client *client, const char *data, rt_size_t size);

//static void udp_recv_data_callback(const char *data, uint16_t len);

#endif