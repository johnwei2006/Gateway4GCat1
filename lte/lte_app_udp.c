#include "ec20_hal.h"
#include "ec20_net.h"
#include "lte_app.h"
#include "lte_def.h"
#include "ec20_protocol_udp.h"
#include "ec20_protocol_http.h"
#include <string.h>
#include <stdlib.h>
#include "../transport/transport_message_queue.h"
#include <utils.h>
// 应用层私有数据结构

static at_response_t atResp;

static rt_mutex_t udp_app_mutex;        // 线程安全锁
static lte_info_t lte_info = {0};         // 静态实例
static uint8_t raw_udp_payload_length=0;
rt_thread_t uplink_message_send_thread = RT_NULL;
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
  //lte_commands_port_t httpCommands; //Http命令接口
} lte_port_t;

static lte_port_t ltePort = {
    .LTE_Boot       = EC20_Boot,
    .LTE_Reboot     = EC20_Reboot,
    .netCommands    = {
        .LTE_GetCommands        = EC20_GetNetCommands,
        .LTE_SetCommandsFirst   = EC20_SetNetCommandsFirst,
    },
    .udp_connect_commands = {
        .LTE_GetCommands        = ec20_get_udp_connect_commands,
        .LTE_SetCommandsFirst   = ec20_set_udp_commands_first,
    },
    .udp_send_data_commands = {
        .LTE_GetCommands        = ec20_get_udp_send_data_commands,
        .LTE_SetCommandsFirst   = ec20_set_udp_commands_first,
    },
};

static struct at_urc urc_table[] = {
 //   {">",                          "\r\n",        urc_send_data_handler},
//    {"+QIURC: ",           			   "\r\n",        urc_recv_data_handler},
    {"+QIURC: \"recv\"",          "\r\n",        urc_recv_data_handler},		
    {"+QIRD",                      "\r\n",        urc_rd_data_handler},
    {"+QIURC: \"closed\"",         "\r\n",        urc_closed_handler},
    //{"+QIURC: \"dnsgip\"",         "\r\n",        urc_dnsgip_handler},		
};

static void module_reboot(void)
{
  ltePort.LTE_Boot();
  ltePort.netCommands.LTE_SetCommandsFirst(atResp);
  ltePort.udp_connect_commands.LTE_SetCommandsFirst(atResp);
  ltePort.udp_send_data_commands.LTE_SetCommandsFirst(atResp);
  //ltePort.httpCommands.LTE_SetCommandsFirst(atResp);
  //networkState = NETWORK_STATE_NOT_OK;
  LOG_EC20("lte module rebooted.\r\n");
}

// 处理UDP接收数据回调 (两种接收方式之一：1、udp_recv_data_callback；2、urc_rd_data_handler)
 static void udp_recv_data_callback(const char *data, uint16_t len){
    raw_udp_message_t *raw_udp_message = (raw_udp_message_t *)rt_malloc(sizeof(raw_udp_message_t));
    if (!raw_udp_message) {
         return;
    }
    raw_udp_message->payload = (char*)rt_malloc(len);
    if (!raw_udp_message->payload) {
        rt_free(raw_udp_message);
        return;
    }
    rt_memcpy(raw_udp_message->payload, data, len);
    raw_udp_message->payload_length = len;
    rt_err_t ret = put_cloud_downlink_message(raw_udp_message);
    if(ret != RT_EOK){
        LOG_CLOUD("put_cloud_downlink_message failed.\r\n");
        rt_free(raw_udp_message->payload);
        rt_free(raw_udp_message);
    }
 }

// urc开始接收数据：+QIURC: "recv"
// 1、+QIURC: "recv" 触发时，发送UDP接收数据指令：QIRD:
// 2、+QIRD: 触发时，解析数据并调用udp_recv_data_callback或urc_recv_data_handler处理
static void urc_recv_data_handler(struct at_client *client, const char *data, rt_size_t size) {
    if (!data || size == 0) {
        return;
    }

    rt_kprintf("+QIURC:'recv' urc_recv_data_handler: %s, %d.\r\n", data, size);
    
    // 缓存模式下执行：AT+QIRD， 直吐模式下不需要主动读数据：以下逻辑无需执行    
    at_command_process_t **udp_read_data_commands = ec20_get_udp_read_data_commands();
    if (!udp_read_data_commands) {
        return;
    }

    // 发送UDP接收数据指令：AT+QIRD:
    lte_command_proccess(udp_read_data_commands);
 }

 // urc接收回调（处理原始数据并转发给业务层）:+QIRD: (两种接收方式之一：1、udp_recv_data_callback；2、urc_rd_data_handler)
static void urc_rd_data_handler(struct at_client *client, const char *data, rt_size_t len) {
    rt_kprintf("+QIRD urc_rd_data_handler: %s, %d.\r\n", atResp->buf, atResp->buf_len);
  
    // 填充UDP消息结构体
    raw_udp_message_t *raw_udp_message = (raw_udp_message_t *)rt_malloc(sizeof(raw_udp_message_t));
  
    if(data[len-2] == '\r' && data[len-1] == '\n'){
       raw_udp_message->payload_length = len-2; 
    } else {
       raw_udp_message->payload_length = len;      
    }
    raw_udp_message->payload = (char*)rt_malloc(raw_udp_message->payload_length);
    rt_memcpy(raw_udp_message->payload, data, raw_udp_message->payload_length);	

		//print_hex_string(raw_udp_message->payload, raw_udp_message->payload_length);
    rt_err_t ret = put_cloud_downlink_message(raw_udp_message);
    if(ret != RT_EOK){
        LOG_CLOUD("put_cloud_downlink_message failed.\r\n");
    }
}

// urc发送数据回调（处理原始数据并转发给业务层）：>
static void urc_send_data_handler(struct at_client *client, const char *data, rt_size_t size){
    LOG_EC20("urc_send_data_handler, AT+QISEND then got return value '>'\r\n");
}
 static void urc_closed_handler(struct at_client *client, const char *data, rt_size_t size){
    LOG_EC20("urc_closed_handler, UDP connection closed.\r\n");
    
    if (lte_info.lte_state == UDP_STATE_CONNECTED) {
        lte_info.lte_state = UDP_STATE_DISCONNECTED;
    }
 }

// 初始化UDP应用
int udp_app_init(const char *server_ip, uint16_t server_port) {
    if (!server_ip || server_port == 0) {
        return -1;
    }

    // 初始化互斥锁
    udp_app_mutex = rt_mutex_create("udp_app_mutex", RT_IPC_FLAG_FIFO);
    if (!udp_app_mutex) {
        return -2;
    }
    
    // 保存服务器信息
    rt_strncpy(lte_info.server_ip, server_ip, sizeof(lte_info.server_ip) - 1);
    lte_info.server_port = server_port;

    // 初始化状态
    lte_info.lte_state = UDP_STATE_INITED;
    lte_info.last_error = 0;


    //ec20_set_udp_recv_callback(udp_recv_data_callback);
    
 	  rt_err_t ret = RT_EOK;
    if(uplink_message_send_thread == RT_NULL){
        uplink_message_send_thread = rt_thread_create("uplink_message_send_thread", udp_app_uplink_process, RT_NULL, RAW_UDP_THREAD_STACK_SIZE, RAW_UDP_THREAD_PRIORITY, RAW_UDP_THREAD_TIMESLICE);
        if (uplink_message_send_thread != RT_NULL){
            rt_thread_startup(uplink_message_send_thread);
        } else {
						LOG_CLOUD("Failed to create uplink_message_send_thread!\n");
            ret = RT_ERROR;
				}
    }
    LOG_EC20("UDP app init success.\r\n");
    return 0;
}

// 启动UDP连接
int udp_app_connect(rt_bool_t is_reconnect, void (*connect_callback)(lte_state_t lte_status)) {
    module_reboot();
    EC20_SimDetection();
    rt_mutex_take(udp_app_mutex, RT_WAITING_FOREVER);

    if (lte_info.lte_state != UDP_STATE_INITED && lte_info.lte_state != UDP_STATE_DISCONNECTED) {
        rt_mutex_release(udp_app_mutex);
        if (connect_callback != RT_NULL) {
            connect_callback(lte_info.lte_state);
        }
        return -1;
    }
    lte_info.lte_state = UDP_STATE_CONNECTING;
    
    // 设置服务器信息并启动连接流程
    char *server_ip_ptr = lte_info.server_ip;
    ec20_set_udp_server(&server_ip_ptr, lte_info.server_port);
    
    // 初始化底层命令
    atResp = EC20_GetAtResp();    
    /* register network vaild*/
    if (lte_command_proccess(ltePort.netCommands.LTE_GetCommands()) == 0) 
    {
        lte_info.ec20_csq = EC20_GetCSQ();
        lte_info.ec20_imei = (char *)EC20_GetIMEI();
        lte_info.ec20_iccid = (char *)EC20_GetICCID();
        lte_info.lte_time = *(EC20_GetTime());
        lte_info.ec20_csq = EC20_GetCSQ();
        lte_info.ec20_imsi = (char *)EC20_GetIMSI();
        lte_info.ec20_operator = (char *)EC20_GetOperator();
        lte_info.ec20_gmi = (char *)EC20_GetGMI();
        lte_info.ec20_gmm = (char *)EC20_GetGMM();
        lte_info.ec20_gmr = (char *)EC20_GetGMR();
        LOG_EC20("LTE GMI: %s.\r\n", lte_info.ec20_gmi);
        LOG_EC20("LTE GMM: %s.\r\n", lte_info.ec20_gmm);
        LOG_EC20("LTE GMR: %s.\r\n", lte_info.ec20_gmr);
        LOG_EC20("LTE IMEI: %s.\r\n", lte_info.ec20_imei);
        LOG_EC20("LTE ICCID: %s.\r\n", lte_info.ec20_iccid);
        LOG_EC20("LTE IMSI: %s.\r\n", lte_info.ec20_imsi);
        LOG_EC20("LTE operator: %s.\r\n", lte_info.ec20_operator);
        LOG_EC20("LTE CSQ: %d.\r\n", lte_info.ec20_csq);
        LOG_EC20("LTE Time: %04d-%02d-%02d %02d:%02d:%02d.\r\n", 
                 lte_info.lte_time.year, lte_info.lte_time.month, lte_info.lte_time.day,
                 lte_info.lte_time.hour, lte_info.lte_time.minute, lte_info.lte_time.second);
    } else {
        lte_info.last_error = 0x01; // 网络注册失败
        lte_info.lte_state = UDP_STATE_DISCONNECTED;
        LOG_EC20("Network registration failed.\r\n");
        rt_mutex_release(udp_app_mutex);
        return -2;
    }

    ec20_set_udp_commands_first(atResp);
    /* udp connect vaild*/
    if(lte_command_proccess(ltePort.udp_connect_commands.LTE_GetCommands()) == 0){
        lte_info.lte_state = UDP_STATE_CONNECTED;
        LOG_EC20("UDP connected to %s:%d.\r\n", lte_info.server_ip, lte_info.server_port);
    } else {
        lte_info.last_error = 0x02; // UDP连接失败
        lte_info.lte_state = UDP_STATE_DISCONNECTED;
        LOG_EC20("UDP connection failed.\r\n");
        rt_mutex_release(udp_app_mutex);
        if (connect_callback != RT_NULL) {
            connect_callback(lte_info.lte_state);
        }
        return -3;
    }
		
		int urc_set_result = at_set_urc_table(urc_table, sizeof(urc_table) / sizeof(urc_table[0]));
    if(urc_set_result != RT_EOK){
        LOG_EC20("Failed to set URC table.\r\n");
        rt_mutex_release(udp_app_mutex);
        if (connect_callback != RT_NULL) {
            connect_callback(lte_info.lte_state);
        }
        return -3;
    }
    rt_mutex_release(udp_app_mutex);
    if (connect_callback != RT_NULL) {
        connect_callback(lte_info.lte_state);
    }
    return 0;
}

// 发送业务数据
int udp_app_send(const char *data, uint16_t len) {
    if (!data || len == 0) {
        return -1;
    }
    atResp = EC20_GetAtResp();
    rt_mutex_take(udp_app_mutex, RT_WAITING_FOREVER);
    if (lte_info.lte_state != UDP_STATE_CONNECTED) {
        LOG_EC20("No UDP connection, cannot send data.\r\n");
        rt_mutex_release(udp_app_mutex);
        return -2;
    }
    // 调用udp协议层发送数据准备
    int ret = ec20_prepare_send_data(data,len);
    if (ret != 0) {
        lte_info.last_error = 0x10; // 发送失败
        lte_info.lte_state = UDP_STATE_DISCONNECTED;
        LOG_EC20("UDP send failed, error: %d.\r\n", ret);
    }	 
    // 调用udp协议层发送数据命令
    lte_command_proccess(ltePort.udp_send_data_commands.LTE_GetCommands());
    rt_mutex_release(udp_app_mutex);
    return ret;
}

// 强制重连
int udp_app_reconnect(void) {
    rt_mutex_take(udp_app_mutex, RT_WAITING_FOREVER);

    // 重置状态
    lte_info.lte_state = UDP_STATE_DISCONNECTED;
    
    // 重新连接
    int ret = udp_app_connect();

    rt_mutex_release(udp_app_mutex);
    return ret;
}

// 释放资源
void udp_app_deinit(void) {
    rt_mutex_take(udp_app_mutex, RT_WAITING_FOREVER);

    // 释放互斥锁
    if (udp_app_mutex) {
        rt_mutex_delete(udp_app_mutex);
        udp_app_mutex = RT_NULL;
    }

    // 重置状态
    lte_info.lte_state = UDP_STATE_UNINIT;
    lte_info.last_error = 0;

    LOG_EC20("UDP app deinit success.\r\n");

    rt_mutex_release(udp_app_mutex);
}

static void udp_app_uplink_process(void* data) {
    // This function should implement the logic to process uplink messages
    while (1) {
        // Placeholder for actual uplink processing logic
        raw_udp_message_t *raw_udp_message = RT_NULL;
        // 从队列中获取消息
        if (get_cloud_uplink_message(&raw_udp_message) == RT_EOK) {
            if(raw_udp_message == RT_NULL) {
                rt_thread_mdelay(100);
                continue;
            }

            //LOG_CLOUD("raw_udp_message->payload: %s, raw_udp_message->payload_length: %d.\r\n", raw_udp_message->payload, raw_udp_message->payload_length);
						//print_hex_string(raw_udp_message->payload, raw_udp_message->payload_length);
            // 发送消息
            udp_app_send(raw_udp_message->payload, raw_udp_message->payload_length);
            // 释放消息结构体内存
          
          free_raw_udp_message(raw_udp_message);
        } 
        rt_thread_mdelay(100);
    }
}   


void printatresp(char *data,rt_size_t len){
for (int i = 0; i < len; i++)
			{
					if (data[i] >= 32 && data[i] <= 126)
							rt_kprintf("%c", data[i]);
					else
							rt_kprintf("[0x%02X]", (unsigned char)data[i]);
			}
			rt_kprintf("\n");
}	

void get_lte_info(lte_info_t* info){
    if(info != RT_NULL){
        rt_memcpy(info, &lte_info, sizeof(lte_info_t));
    }
}

static int lte_command_proccess(at_command_process_t **pCommands)
{
	atResp=EC20_GetAtResp();
  if (atResp == NULL)
  {
    rt_kprintf("lte_app.lte_command_proccess: LTE atResp is NULL.\r\n");
    return -1;
  }
  if ((*pCommands) == NULL) //允许命令为空
  {
    //    LOG_LTE("LTE commands error");
    return -2;
  }
  do
  {
    int tickMs = 0;
    int cmdCount = 0;                                       //命令发送计数
    for (uint8_t com = 0; com < (*pCommands)->commandSize;) //执行命令组
    {
      static uint8_t paramInit = 0;
      static char *sendBuffer = RT_NULL;
      if (paramInit == 0) //初始化命令
      {
        char *param = RT_NULL;
        int strLength = 0;
        paramInit = 1;
        //设置预期回复参数
        at_resp_set_info(atResp, DEFAULT_AT_RESPONE_BUFFER_SIZE, DEFAULT_AT_RESPONE_LINE_NUMBER, (*pCommands)->pCommand[com].timeout);
        strLength = rt_strlen((*pCommands)->pCommand[com].cmd);
        //组合命令参数
        if ((*pCommands)->pCommand[com].get_param != RT_NULL) //命令存在参数
        {
          param = (*pCommands)->pCommand[com].get_param();
        }
        if (sendBuffer != RT_NULL)
        {
          rt_free(sendBuffer);
          sendBuffer = RT_NULL;
        }
        if (param)
        strLength += rt_strlen(param);
        sendBuffer = (char *)rt_malloc(strLength + 1);
        if (sendBuffer == RT_NULL)
        {
          rt_kprintf("lte_app.lte_command_proccess: malloc sendBuffer error.\r\n");
          com = 0;
          cmdCount = 0;
          paramInit = 0;
          EC20_Reboot();
          return -3;
        }
        rt_strncpy(sendBuffer, (*pCommands)->pCommand[com].cmd, rt_strlen((*pCommands)->pCommand[com].cmd) + 1);
        if (param) //命令存在参数
        {
          strcat(sendBuffer, (*pCommands)->pCommand[com].get_param());
        }
        tickMs = rt_tick_get_millisecond(); //获取当前计数值
      }
      rt_kprintf("lte_app.lte_command_proccess to send   : %s \r\n", sendBuffer);
      //发送命令
      at_exec_cmd(atResp, sendBuffer);

      rt_kprintf("\r\natResponse length %d bytes:\n", atResp->buf_len);
      rt_kprintf("---------------------------------------------\n");
      printatresp(atResp->buf,atResp->buf_len);
      rt_kprintf("---------------------------------------------\n");
			
      //打印命令  
      rt_kprintf("lte_app.lte_command_proccess to receive: %d/%d lines\r\n", atResp->line_counts, atResp->line_num);
      for (int i = 0; i <= atResp->line_counts; i++)
      {
        rt_kprintf("   line %d: %s\r\n", i, at_resp_get_line(atResp, i));
      }
      if ((*pCommands)->pCommand[com].expect_response != RT_NULL && at_resp_get_line_by_kw(atResp, (*pCommands)->pCommand[com].expect_response) == RT_NULL) //收不到指定返回
      {
        if (++cmdCount >= AT_COMMAND_RETRY_MAX_COUNT) //尝试次数过多
        {
          com = 0;
          cmdCount = 0;
          paramInit = 0;
          lte_info.lte_state = UDP_STATE_UNINIT;
          rt_kprintf("lte_app.lte_command_proccess: at cmd max retry");
          EC20_Reboot();
          return -2;
        }
        continue;
      }
      //执行命令处理
      if ((*pCommands)->pCommand[com].command_handle != RT_NULL && (*pCommands)->pCommand[com].command_handle() != AT_COMMAND_HANDLE_STATE_SUCCESS)
      {
        if (rt_tick_get_millisecond() - tickMs > (int)(*pCommands)->waitSecond * 1000) //判断命令是否超时
        {
          com = 0;
          cmdCount = 0;
          paramInit = 0;
          rt_kprintf("lte_app.lte_command_proccess: at cmd handle error.");
          EC20_Reboot();
          return -3;
        }
        continue;
      }		
			rt_memset(atResp->buf,0,atResp->buf_len);
      com++; //执行下一条
      cmdCount = 0;
      paramInit = 0;
      if (sendBuffer != RT_NULL)
      {
        rt_free(sendBuffer);
        sendBuffer = RT_NULL;
      }
    }

    if ((*pCommands)->ok_function != RT_NULL)
      (*pCommands)->ok_function(); //执行成功

    (*pCommands) = (*pCommands)->next; //执行下一个命令组

  } while ((*pCommands) != RT_NULL);
  LOG_CLOUD("lte_app.lte_command_proccess: all commands processed successfully.\r\n");
  return 0;
}