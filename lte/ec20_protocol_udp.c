/*
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe: EC20 UDP functionality implementation
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
 * 2024-05-20     user           first version
 */
#include "ec20_protocol_udp.h"
#include "ec20_hal.h"
#include "lte_def.h"
#include <rtthread.h>
#include <string.h>
#include <stdio.h>
#include "lte_app.h"
#define ARRAY_SIZE(a)		(sizeof(a)/sizeof(a[0]))
	
// AT指令参数
static char *udpServer = RT_NULL;
static uint16_t udpPort = 0;
static char connect_server_info[64] = {0};  // 用于连接服务器时拼接服务器信息，包括：地址和端口信息
static char send_data_server_info[100] = {0};    // 用于发送数据时拼接服务器信息，包括：地址、端口和数据长度
static char sendBuffer[1024] = {0}; // 发送数据缓冲区
rt_size_t udp_send_len;
static char udp_send_len_char[4] = {0}; // 用于存储udp_send_len的4字节表示
// AT命令组指针
static at_response_t atResp;
static at_command_process_t *at_command_array = RT_NULL;
// UDP接收回调函数
static udp_recv_callback udp_receive_function = RT_NULL;

// 注册AT参数相关函数
CREAT_AT_COMMAND_PARAM_FUNCTION(udpServer);
CREAT_AT_COMMAND_PARAM_FUNCTION(connect_server_info);
CREAT_AT_COMMAND_PARAM_FUNCTION(send_data_server_info);
CREAT_AT_COMMAND_PARAM_FUNCTION(sendBuffer); 
CREAT_AT_COMMAND_PARAM_FUNCTION(udp_send_len_char); 

// UDP连接处理:AT+QIOPEN=1,2,\"UDP SERVICE\",
static at_command_handle_state_t at_connect_callback(void){
		int connect_result = 0;
		int connect_id     = 0;
		int matched_line_number = at_resp_parse_line_args_by_kw(atResp, "+QIOPEN:", "+QIOPEN: %d,%d", &connect_id, &connect_result);
		if (matched_line_number == -1)
		{
			LOG_EC20("QIOPEN error");
			return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
		}

		//const char *resp = at_resp_get_line_by_kw(atResp, "+QIOPEN: \s*\d+,\s*0");
    //const char *resp = at_resp_get_line_by_kw(atResp, AT_RESP_OK_STRING);
    if (connect_result == 0)
    {
        LOG_EC20("UDP connected successfully");
        set_lte_status(UDP_STATE_CONNECTED);
        return AT_COMMAND_HANDLE_STATE_SUCCESS;
    }
    set_lte_status(UDP_STATE_DISCONNECTED);
    return AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
}
// UDP发送命令处理:AT+QISEND=2,
static at_command_handle_state_t at_send_command_callback(void){
	
    const char *resp = at_resp_get_line_by_kw(atResp, AT_RESP_ERROR_STRING);
    if (resp){
        LOG_EC20("Failed to send UDP data");
        set_lte_status(UDP_STATE_DISCONNECTED);
        return AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
    }
    rt_size_t udp_size = at_client_send(sendBuffer, udp_send_len);
    LOG_EC20("at_client_send UDP send %d bytes", udp_size);
    if (udp_size > 0){

        return AT_COMMAND_HANDLE_STATE_SUCCESS;
    }
    return AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
};

// UDP接收数据处理:AT+QIRD=2,2048
static at_command_handle_state_t at_recv_data_callback(void)
{
	 const char *data = (char *)at_resp_get_line(atResp, 2);

   if (data && udp_receive_function)
   {
      udp_receive_function(data, strlen(data));
	 }
   return AT_COMMAND_HANDLE_STATE_SUCCESS;
}

static at_command_handle_state_t at_domain_name_resolve_callback(void)
{
    const char *resp = at_resp_get_line_by_kw(atResp, "+QIURC: \"dnsgip\",\"");
    if (resp)
    {
        LOG_EC20("Domain name resolved successfully");
        char* host_ip = strstr(resp, "dnsgip");
        if (host_ip)
        {
            host_ip += strlen("dnsgip\",\"");
            host_ip = strtok(host_ip, "\"");
            if (host_ip)
            {
                LOG_EC20("Host IP:%s", host_ip);
            }
        }
        ec20_set_udp_server(&host_ip, udpPort);
        return AT_COMMAND_HANDLE_STATE_SUCCESS;
    }
    return AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
}

static at_command_handle_state_t at_connect_check_callback(void)
{  
    const char *resp = at_resp_get_line_by_kw(atResp, "+QISTATE: 2");
    if (resp)
    {
        LOG_EC20("UDP connected successfully");
        set_lte_status(UDP_STATE_CONNECTED);
        return AT_COMMAND_HANDLE_STATE_SUCCESS;
    }
     set_lte_status(UDP_STATE_DISCONNECTED);
     LOG_EC20("UDP not connected");
    return AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
}
static at_command command_udp_connect[] = {
    {"AT+QURCCFG=\"urcport\",\"uart1\"", RT_NULL, 1000, RT_NULL, RT_NULL},   
    //{"AT+QPING=1,", AT_COMMAND_PARAM_FUNCTION(udpServer), 3000, RT_NULL, at_domain_name_resolve_callback}, //domain name resolution
    {"AT+QIDNSGIP=1,", AT_COMMAND_PARAM_FUNCTION(udpServer), 5000, RT_NULL, at_domain_name_resolve_callback},
	//{"AT+QIOPEN=1,2,\"UDP SERVICE\",", AT_COMMAND_PARAM_FUNCTION(connect_server_info), 3000, AT_RESP_OK_STRING, at_connect_callback}, // UDP SERVICE
    //{"AT+QIOPEN=1,2,\"UDP\",", AT_COMMAND_PARAM_FUNCTION(connect_server_info), 3000, AT_RESP_OK_STRING, at_connect_callback}, // UDP
    {"AT+QIOPEN=1,2,\"TCP\",", AT_COMMAND_PARAM_FUNCTION(connect_server_info), 5000, AT_RESP_OK_STRING, at_connect_callback}, // TCP
    //{"AT+QURCCFG=\"recv\",1", RT_NULL, 1000, AT_RESP_OK_STRING, RT_NULL},
    {"AT+QISTATE?", RT_NULL, 1000, RT_NULL, RT_NULL}
};
// UDP发送数据  
static at_command command_udp_send_command[] = {
	//{"AT+QISEND=2,", AT_COMMAND_PARAM_FUNCTION(send_data_server_info), 3000, RT_NULL, at_send_command_callback},	//"UDP SERVICE" mode
    {"AT+QISEND=2,", AT_COMMAND_PARAM_FUNCTION(udp_send_len_char), 3000, RT_NULL, at_send_command_callback}         //"UDP mode with fixed data length
    //{"AT+QISEND=2", RT_NULL, 3000, RT_NULL, at_send_command_callback},                      	                    //"UDP mode with unfixed data length
    //{"AT+QISEND=2,0", RT_NULL, 3000, RT_NULL, RT_NULL},         	                                                //check sent data size
};

// UDP接收数据  
static at_command command_udp_read_command[] = {
	  {"AT+QIRD=2", RT_NULL, 1000, RT_NULL, RT_NULL}
    //{"AT+QIRD=2", RT_NULL, 1000, RT_NULL, at_recv_data_callback},	
};

static at_command command_connect_check[] = {
    {"AT+QISTATE?", RT_NULL, 1000, RT_NULL, at_connect_check_callback}
};

//UDP连接指令处理流程
static at_command_process_t command_connect_process[] = {
  {AT_COMMAND_ARRAY(command_udp_connect), 0, RT_NULL, RT_NULL},  
};

//UDP发送数据指令处理流程
static at_command_process_t command_send_data_process[] = {
  {AT_COMMAND_ARRAY(command_udp_send_command), 0, RT_NULL, RT_NULL}
};

//UDP接收数据指令处理流程
static at_command_process_t command_recv_data_process[] = {
   {AT_COMMAND_ARRAY(command_udp_read_command), 0, RT_NULL, RT_NULL}
};

static at_command_process_t command_check_connect_process[] = {
   {AT_COMMAND_ARRAY(command_connect_check), 0, RT_NULL, RT_NULL}
};
//设置UDP服务器地址和端口
void ec20_set_udp_server(char **server, uint16_t port)
{
    if (*server == RT_NULL) return;    
    udpServer = *server;
    udpPort = port;
    rt_memset(connect_server_info, 0, sizeof(connect_server_info));
    rt_snprintf(connect_server_info,sizeof(connect_server_info),"\"%s\",%d,0,0", udpServer, udpPort); //0-cache mode
    //rt_snprintf(connect_server_info,sizeof(connect_server_info),"\"%s\",%d,0,1", udpServer, udpPort); //1-direct mode
}
// 设置初始UDP命令
void ec20_set_udp_commands_first(at_response_t resp){
	  atResp=EC20_GetAtResp();
    LOG_EC20("UDP set first command");	
}

// 获取UDP连接命令组
at_command_process_t **ec20_get_udp_connect_commands(void){
    at_command_array = &command_connect_process[0];
    return &at_command_array;
}

// 获取UDP发送数据命令组
at_command_process_t **ec20_get_udp_send_data_commands(void){
    at_command_array = &command_send_data_process[0];
    return &at_command_array;
}

at_command_process_t **ec20_get_udp_read_data_commands(void){
    at_command_array = &command_recv_data_process[0];
    return &at_command_array;
}
// 获取UDP连接检查命令组
at_command_process_t **ec20_get_udp_check_connect_commands(void){
    at_command_array = &command_check_connect_process[0];
    return &at_command_array;
}
// 设置UDP接收回调函数
void ec20_set_udp_recv_callback(udp_recv_callback function){
    udp_receive_function = function;
}
// 发送UDP数据
int ec20_prepare_send_data(const char *data, uint16_t len){
    if (data == RT_NULL || len == 0)
      return -1;    
    rt_memset(sendBuffer, 0, sizeof(sendBuffer));
    rt_memcpy(sendBuffer, data, len);
    //rt_kprintf("sendBuffer hex:\n");
    // print_hex_string(sendBuffer, len);
	udp_send_len = len;
    sprintf(udp_send_len_char, "%u", udp_send_len);
	rt_memset(send_data_server_info, 0, sizeof(send_data_server_info));
	rt_snprintf(send_data_server_info,sizeof(send_data_server_info),"%d,\"%s\",%d", udp_send_len, udpServer, udpPort);
    return 0;
}