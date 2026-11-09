/*
* Copyright (c) 2020-2021, Kebai Development Team
* Describe:
* License-Identifier: KB_Base_RTT
* Change Logs:
* Date          Author         Notes
* 2021-6-28     magnin          the first version
*/
#include "ec20_net.h"
#include "ec20_protocol_http.h"
#include "ec20_hal.h"
#include <rtthread.h>
#include <string.h>
#include <stdio.h>

#define ARRAY_SIZE(a)					(sizeof(a)/sizeof(a[0]))
	
//AT指令参数
static char *serverUrl = RT_NULL;
static char serverUrlLength[16] = {0};
//AT回复结构指针
static at_response_t atResp = RT_NULL;
//AT命令组指针
static at_command_process_t *pComPro = RT_NULL;
static http_read_callback read_callback = RT_NULL;
//注册AT参数相关函数
CREAT_AT_COMMAND_PARAM_FUNCTION(serverUrl);
CREAT_AT_COMMAND_PARAM_FUNCTION(serverUrlLength);

// QHTTPREAD 响应参数
static uint32_t http_read_offset = 0;
static uint32_t http_read_len    = 0;
static uint32_t http_remain_len  = 0;
static char httpReadLenStr[16]   = "10";
static char httpGetParamStr[32]  = "10";         
CREAT_AT_COMMAND_PARAM_FUNCTION(httpReadLenStr);
CREAT_AT_COMMAND_PARAM_FUNCTION(httpGetParamStr); 

static at_command_handle_state_t http_get_ok(void)
{
    rt_thread_mdelay(500);
    return AT_COMMAND_HANDLE_STATE_SUCCESS;
}

//读取数据
// 读取数据
static at_command_handle_state_t http_read_ok(void)
{
    // 解析 QHTTPREAD 响应行，提取 offset, read_len, remain_len
    const char* read_line = at_resp_get_line_by_kw(atResp, "QHTTPREAD");
    if (read_line)
    {
        int offset = 0, read_len = 0, remain_len = 0;
        if (sscanf(read_line, "%*[^:]: %d,%d,%d", &offset, &read_len, &remain_len) == 3)
        {
            http_read_offset = (uint32_t)offset;
            http_read_len    = (uint32_t)read_len;
            http_remain_len  = (uint32_t)remain_len;
            rt_kprintf("HTTP read: offset=%u, read_len=%u, remain_len=%u\n", 
                       http_read_offset, http_read_len, http_remain_len);
        }
        else if (sscanf(read_line, "%*[^:]: %d", &read_len) == 1)
        {
            http_read_len    = (uint32_t)read_len;
            http_read_offset = 0;
            http_remain_len  = 0;
            rt_kprintf("HTTP read: data_len=%u\n", http_read_len);
        }
    }

    // 获取所有响应行，找到 CONNECT 和 OK 的位置
    rt_int32_t line_count = atResp->line_counts;
    rt_int32_t connect_line = -1;
    rt_int32_t ok_line = -1;
    rt_int32_t i;

    for (i = 0; i < line_count; i++)
    {
        const char* line = at_resp_get_line(atResp, i);
        if (line == RT_NULL) continue;

        if (connect_line < 0 && strstr(line, "CONNECT"))
        {
            connect_line = i;
        }
        else if (connect_line >= 0 && strstr(line, "OK"))
        {
            ok_line = i;
            break;
        }
    }

    if (connect_line >= 0 && ok_line > connect_line)
    {
        // 使用 rt_memcpy 拼接，避免二进制数据中的 \0 截断
        static char data_buffer[2048];
        uint32_t data_len = 0;
        rt_memset(data_buffer, 0, sizeof(data_buffer));

        for (i = connect_line + 1; i < ok_line; i++)
        {
            const char* line = at_resp_get_line(atResp, i);
            if (line)
            {
                uint32_t line_len = rt_strlen(line);
                if (data_len + line_len < sizeof(data_buffer) - 1)
                {
                    rt_memcpy(data_buffer + data_len, line, line_len);
                    data_len += line_len;
                }
            }
        }
        data_buffer[data_len] = '\0';

        if (read_callback)
        {
            read_callback(data_buffer);
            return AT_COMMAND_HANDLE_STATE_SUCCESS;
        }
    }

    return AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
}

//配置 HTTP
static at_command commandHttpCFG[] = {
    {"AT+QHTTPCFG=\"contextid\",1",       RT_NULL,             800,  AT_RESP_OK_STRING, RT_NULL},  
};
// http url
static at_command commandHttpUrl[] = {
    {"AT+QHTTPURL=",      AT_COMMAND_PARAM_FUNCTION(serverUrlLength), 1000,  "CONNECT",         RT_NULL}, 
    {"",                  AT_COMMAND_PARAM_FUNCTION(serverUrl),       1000,  AT_RESP_OK_STRING, RT_NULL},
};  
// http get
static at_command commandHttpGet[] = {
    {"AT+QHTTPGET=",      AT_COMMAND_PARAM_FUNCTION(httpGetParamStr), 30000, AT_RESP_OK_STRING, http_get_ok}, 
    {"AT+QHTTPREAD=",     AT_COMMAND_PARAM_FUNCTION(httpReadLenStr),  30000, "CONNECT",         http_read_ok},
};

//指令处理
static at_command_process_t commandProcess[] = {
    //配置
    {AT_COMMAND_ARRAY(commandHttpCFG), 0,   RT_NULL,    &commandProcess[1]},

    {AT_COMMAND_ARRAY(commandHttpUrl), 0,   RT_NULL,    &commandProcess[2]},
    
    {AT_COMMAND_ARRAY(commandHttpGet), 0,   RT_NULL,    RT_NULL},
};
void EC20_SetHttpUrl(char **url)
{
    if(*url == RT_NULL) return;
    //服务器URL
    serverUrl = *url;
    rt_memset(serverUrlLength,0,sizeof(serverUrlLength));
    rt_sprintf(serverUrlLength,"%u,10",rt_strlen(serverUrl));
    if(pComPro == RT_NULL) pComPro = &commandProcess[1];
}
//设置
void EC20_SetHttpCommandsFirst(at_response_t resp)
{
    atResp = resp;
    pComPro = &commandProcess[0];
    LOG_EC20("HTTP set http first command");
}
//获取网络命令
at_command_process_t **EC20_GetHttpCommands(void)
{
    return &pComPro;
}
void EC20_SetHttpReadCallback(http_read_callback callback)
{
    read_callback = callback;
}

void EC20_SetHttpReadLen(uint32_t read_len)
{
    rt_memset(httpReadLenStr, 0, sizeof(httpReadLenStr));
    rt_sprintf(httpReadLenStr, "%u", read_len);
    http_read_len    = read_len;
    http_read_offset = 0;
    http_remain_len  = 0;
}

uint32_t EC20_GetHttpReadOffset(void)
{
    return http_read_offset;
}

uint32_t EC20_GetHttpReadLen(void)
{
    return http_read_len;
}

uint32_t EC20_GetHttpRemainLen(void)
{
    return http_remain_len;
}
void EC20_SetHttpGetParam(uint32_t timeout)
{
    rt_memset(httpGetParamStr, 0, sizeof(httpGetParamStr));
    rt_sprintf(httpGetParamStr, "%u", timeout);
}
uint32_t EC20_AdvanceHttpOffset(void)
{
    http_read_offset += http_read_len;
    rt_kprintf("HTTP offset advanced to: %u\n", http_read_offset);
    return http_read_offset;
}