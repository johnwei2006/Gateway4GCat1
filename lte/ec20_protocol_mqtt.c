/*
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe: EC20 MQTT functionality implementation
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
 * 2024-05-23     user           first version
 */
#include "ec20_protocol_mqtt.h"
#include "ec20_hal.h"
#include "lte_def.h"
#include <rtthread.h>
#include <string.h>
#include <stdio.h>
#define ARRAY_SIZE(a)		(sizeof(a)/sizeof(a[0]))

//AT指令参数
static char version[] = "4";
static char ssl[] = "1,0";
static char keepalive[5] = "120";
static char session[] = "1";
static char *will = RT_NULL;

static char *mqttServer = RT_NULL;
static uint16_t mqttPort = 0;
static char *mqttClientId = RT_NULL;
static char *mqttUsername = RT_NULL;
static char *mqttPassword = RT_NULL;
static char open_mqtt_info[128] = {0};  // 用于连接MQTT服务器时拼接服务器信息
static char connect_mqtt_info[128] = {0};  // 用于连接MQTT服务器时拼接服务器信息
static char publish_mqtt_info[200] = {0}; // 用于发布消息时拼接信息
static char sendBuffer[2048] = {0}; // 发送数据缓冲区
rt_size_t mqtt_send_len;
static char mqtt_send_len_char[6] = {0}; // 用于存储mqtt_send_len的字符串表示
static uint8_t mqtt_qos = 0;
static char *mqtt_topic = RT_NULL;

// AT命令组指针
static at_response_t atResp;
static at_command_process_t *at_command_array = RT_NULL;

// MQTT接收回调函数
static mqtt_recv_callback mqtt_receive_function = RT_NULL;

// 注册AT参数相关函数
//注册AT参数相关函数
CREAT_AT_COMMAND_PARAM_FUNCTION(version);
CREAT_AT_COMMAND_PARAM_FUNCTION(ssl);
CREAT_AT_COMMAND_PARAM_FUNCTION(keepalive);
CREAT_AT_COMMAND_PARAM_FUNCTION(session);
CREAT_AT_COMMAND_PARAM_FUNCTION(will);

CREAT_AT_COMMAND_PARAM_FUNCTION(mqttServer);
CREAT_AT_COMMAND_PARAM_FUNCTION(open_mqtt_info);
CREAT_AT_COMMAND_PARAM_FUNCTION(connect_mqtt_info);
CREAT_AT_COMMAND_PARAM_FUNCTION(publish_mqtt_info);
CREAT_AT_COMMAND_PARAM_FUNCTION(sendBuffer); 
CREAT_AT_COMMAND_PARAM_FUNCTION(mqtt_send_len_char); 

// MQTT连接处理: AT+QMTCONN=0,"ClientID","UserName","Password"
static at_command_handle_state_t at_mqtt_connect_callback(void)
{  
    const char *resp = at_resp_get_line_by_kw(atResp, "+QMTCONN:");
    if (resp && strstr(resp, ",0,0"))
    {
        LOG_EC20("MQTT connected successfully");
        return AT_COMMAND_HANDLE_STATE_SUCCESS;
    }
    return AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
}

// MQTT发布消息处理
static at_command_handle_state_t at_mqtt_publish_callback(void){
    rt_size_t mqtt_size = at_client_send(sendBuffer, mqtt_send_len);
    LOG_EC20("at_client_send MQTT publish %d bytes", mqtt_size);
    if (mqtt_size > 0){
        return AT_COMMAND_HANDLE_STATE_SUCCESS;
    }
    return AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
}

// MQTT接收数据处理
static at_command_handle_state_t at_mqtt_recv_callback(void)
{
	 const char *data = (char *)at_resp_get_line(atResp, 2);

   if (data && mqtt_receive_function)
   {
      mqtt_receive_function(data, strlen(data));
	 }
   return AT_COMMAND_HANDLE_STATE_SUCCESS;
}

// MQTT配置命令组
static at_command command_mqtt_config[] = {
    {"AT+QMTCFG=\"version\",0,",      AT_COMMAND_PARAM_FUNCTION(version),   800, AT_RESP_OK_STRING, RT_NULL}, //mqtt版本
    {"AT+QMTCFG=\"ssl\",0,",          AT_COMMAND_PARAM_FUNCTION(ssl),       800, AT_RESP_OK_STRING, RT_NULL}, //ssl
    {"AT+QMTCFG=\"session\",0,",      AT_COMMAND_PARAM_FUNCTION(session),   800, AT_RESP_OK_STRING, RT_NULL}, //session
    {"AT+QMTCFG=\"keepalive\",0,",    AT_COMMAND_PARAM_FUNCTION(keepalive), 800, AT_RESP_OK_STRING, RT_NULL}, //keepalive
    {"AT+QMTCFG=\"will\",0,",         AT_COMMAND_PARAM_FUNCTION(will),      800, AT_RESP_OK_STRING, RT_NULL}, //will
    {"AT+QMTCFG=\"recv/mode\",0,0,1", RT_NULL,                              800, AT_RESP_OK_STRING, RT_NULL}, //recv
    {"AT+QMTCFG=\"qmtping\",5",       RT_NULL,                              800, AT_RESP_OK_STRING, RT_NULL}, //qmtping
    /*
    {"AT+QMTCFG=\"version\",0,4", RT_NULL, 1000, AT_RESP_OK_STRING, RT_NULL}, // MQTT版本配置
    {"AT+QMTCFG=\"clean\",0,1", RT_NULL, 1000, AT_RESP_OK_STRING, RT_NULL},   // 清理会话
    {"AT+QMTCFG=\"keepalive\",0,60", RT_NULL, 1000, AT_RESP_OK_STRING, RT_NULL}, // 保活时间
    */
};

// MQTT连接命令组
static at_command command_mqtt_connect[] = {
    {"AT+QMTOPEN=0,", AT_COMMAND_PARAM_FUNCTION(open_mqtt_info),    5000, "+QMTOPEN:", at_mqtt_connect_callback}, // 打开MQTT连接
    {"AT+QMTCONN=0,", AT_COMMAND_PARAM_FUNCTION(connect_mqtt_info), 5000, "+QMTCONN:", at_mqtt_connect_callback}, // 连接到MQTT服务器
};

// MQTT发送数据命令组
static at_command command_mqtt_send_command[] = {
    {"AT+QMTPUB=0,1,", AT_COMMAND_PARAM_FUNCTION(publish_mqtt_info), 3000, RT_NULL, at_mqtt_publish_callback}, // 发布消息
};

// MQTT订阅命令组
static at_command command_mqtt_sub_command[] = {
    {"AT+QMTSUB=0,1,", RT_NULL, 3000, RT_NULL, RT_NULL}, // 订阅主题
};

// MQTT取消订阅命令组
static at_command command_mqtt_unsub_command[] = {
    {"AT+QMTUNS=0,1,", RT_NULL, 3000, RT_NULL, RT_NULL}, // 取消订阅
};

// MQTT配置指令处理流程
static at_command_process_t command_config_process[] = {
  {AT_COMMAND_ARRAY(command_mqtt_config), 0, RT_NULL, &command_config_process[1]},  
  {AT_COMMAND_ARRAY(command_mqtt_connect), 0, RT_NULL, RT_NULL},  
};

// MQTT发送数据指令处理流程
static at_command_process_t command_send_data_process[] = {
  {AT_COMMAND_ARRAY(command_mqtt_send_command), 0, RT_NULL, RT_NULL}
};

// MQTT订阅指令处理流程
static at_command_process_t command_sub_process[] = {
  {AT_COMMAND_ARRAY(command_mqtt_sub_command), 0, RT_NULL, RT_NULL}
};

// MQTT取消订阅指令处理流程
static at_command_process_t command_unsub_process[] = {
  {AT_COMMAND_ARRAY(command_mqtt_unsub_command), 0, RT_NULL, RT_NULL}
};

// 设置MQTT服务器参数
void ec20_set_mqtt_params(const char *server_ip, uint16_t port, const char *client_id, const char *username, const char *password)
{
    if (server_ip == RT_NULL) return;
    
    mqttServer = (char*)server_ip;
    mqttPort = port;
    mqttClientId = (char*)client_id;
    mqttUsername = (char*)username;
    mqttPassword = (char*)password;
    
    // 构建打开MQTT server信息
    rt_memset(open_mqtt_info,    0, sizeof(open_mqtt_info));
    rt_sprintf(open_mqtt_info, "\"%s\",%u", mqttServer, mqttPort);

    // 构建连接参数
    rt_memset(connect_mqtt_info, 0, sizeof(connect_mqtt_info));
    if (mqttUsername && mqttPassword) {
        rt_sprintf(connect_mqtt_info, "\"%s\",\"%s\",\"%s\"", mqttClientId, mqttUsername, mqttPassword);//connect_mqtt_info = "\"ClientID\",\"UserName\",\"Password\""
        //rt_snprintf(connect_mqtt_info, sizeof(connect_mqtt_info), "\"%s\",%d,\"%s\",\"%s\",\"%s\"", mqttServer, mqttPort, mqttClientId, mqttUsername, mqttPassword);
    } else {
         rt_sprintf(connect_mqtt_info, "\"%s\"", mqttClientId ? mqttClientId : "default_client");//connect_mqtt_info = "\"ClientID\""
        //rt_snprintf(connect_mqtt_info, sizeof(connect_mqtt_info), "\"%s\",%d,\"%s\"", mqttServer, mqttPort, mqttClientId ? mqttClientId : "default_client");
    }
}

// 设置初始MQTT命令
void ec20_set_mqtt_commands_first(at_response_t resp){
    atResp = EC20_GetAtResp();
    LOG_EC20("MQTT set first command");	
}

// 获取MQTT连接命令组
at_command_process_t **ec20_get_mqtt_connect_commands(void){
    at_command_array = &command_config_process[0];
    return &at_command_array;
}

// 获取MQTT发送数据命令组
at_command_process_t **ec20_get_mqtt_send_data_commands(void){
    at_command_array = &command_send_data_process[0];
    return &at_command_array;
}

at_command_process_t **ec20_get_mqtt_read_data_commands(void){
    // 对于MQTT，我们使用URC机制来接收数据，而不是轮询
    return NULL;
}

// MQTT连接服务器
// 设置MQTT接收回调函数
void ec20_set_mqtt_recv_callback(mqtt_recv_callback function){
    mqtt_receive_function = function;
}

// 准备发布MQTT数据
int ec20_prepare_mqtt_publish_data(const char *topic, const char *data, uint16_t len, uint8_t qos) {
    if (data == RT_NULL || len == 0 || topic == RT_NULL)
      return -1;    
    
    rt_memset(sendBuffer, 0, sizeof(sendBuffer));
    rt_memcpy(sendBuffer, data, len);
    
    mqtt_send_len = len;
    sprintf(mqtt_send_len_char, "%u", mqtt_send_len);
    
    // 构建发布命令参数
    rt_memset(publish_mqtt_info, 0, sizeof(publish_mqtt_info));
    rt_snprintf(publish_mqtt_info, sizeof(publish_mqtt_info), "\"%s\",%d,%d,\"%s\"", 
               topic, qos, 0, data); // 0表示非retain消息
    
    return 0;
}

// 准备订阅MQTT主题
int ec20_prepare_mqtt_subscribe_data(const char *topic, uint8_t qos) {
    if (topic == RT_NULL)
      return -1;    
    
    mqtt_qos = qos;
    mqtt_topic = (char*)topic;
    
    // 构建订阅命令参数
    sprintf(publish_mqtt_info, "\"%s\",%d", mqtt_topic, qos);
    
    return 0;
}

// 准备取消订阅MQTT主题
int ec20_prepare_mqtt_unsubscribe_data(const char *topic) {
    if (topic == RT_NULL)
      return -1;    
    
    mqtt_topic = (char*)topic;
    
    // 构建取消订阅命令参数
    sprintf(publish_mqtt_info, "\"%s\"", mqtt_topic);
    
    return 0;
}