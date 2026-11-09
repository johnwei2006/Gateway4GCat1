#include "ec20_hal.h"
#include "ec20_net.h"
#include "lte_app.h"
#include "lte_def.h"
//#include "ec20_protocol_udp.h"
#include "ec20_protocol_mqtt.h"
#include <string.h>
#include <stdlib.h>
#include "../transport/transport_message_queue.h"
#include <utils.h>
#include <rtdef.h>

// 应用层私有数据结构

static at_response_t atResp;

static rt_mutex_t   mqtt_app_mutex;          // 线程安全锁
static rt_mutex_t   mqtt_receive_mutex;      // receive mqtt mutex
static rt_mutex_t   mqtt_send_mutex;         // send mqtt mutex

static lte_info_t   lte_info = {0};         // 静态实例
static char*        partial_message = RT_NULL;
static uint8_t      partial_message_length=0;

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
  lte_commands_port_t mqtt_connect_commands; //MQTT连接命令接口
  lte_commands_port_t mqtt_send_data_commands; //MQTT发送数据命令接口
  //lte_commands_port_t httpCommands; //Http命令接口
} lte_port_t;

static lte_port_t ltePort = {
    .LTE_Boot       = EC20_Boot,
    .LTE_Reboot     = EC20_Reboot,
    .netCommands    = {
        .LTE_GetCommands        = EC20_GetNetCommands,
        .LTE_SetCommandsFirst   = EC20_SetNetCommandsFirst,
    },
    .mqtt_connect_commands = {
        .LTE_GetCommands        = ec20_get_mqtt_connect_commands,
        .LTE_SetCommandsFirst   = ec20_set_mqtt_commands_first,
    },
    .mqtt_send_data_commands = {
        .LTE_GetCommands        = ec20_get_mqtt_send_data_commands,
        .LTE_SetCommandsFirst   = ec20_set_mqtt_commands_first,
    },
};

static struct at_urc urc_table[] = {
    {"+QIURC: \"recv\"",          "\r\n",        urc_recv_data_handler},		
    {"+QIRD",                      "\r\n",        urc_rd_data_handler},
    {"+QIURC: \"closed\"",         "\r\n",        urc_closed_handler},
    {"+QMTSTAT:",                 "\r\n",        urc_mqtt_status_handler},
    {"+QMTRECV:",                 "\r\n",        urc_mqtt_recv_handler},
};

static void module_reboot(void)
{
  ltePort.LTE_Boot();
  ltePort.netCommands.LTE_SetCommandsFirst(atResp);
  ltePort.mqtt_connect_commands.LTE_SetCommandsFirst(atResp);
  ltePort.mqtt_send_data_commands.LTE_SetCommandsFirst(atResp);
  //ltePort.httpCommands.LTE_SetCommandsFirst(atResp);
  //networkState = NETWORK_STATE_NOT_OK;
  LOG_EC20("lte module rebooted.\r\n");
}

// 处理MQTT接收数据回调
static void mqtt_recv_data_callback(const char *data, uint16_t len){
    raw_udp_message_t *raw_udp_message = (raw_udp_message_t *)rt_malloc(sizeof(raw_udp_message_t));
    if (!raw_udp_message) {
         return;
    }
    raw_udp_message->payload_length = len;        
    raw_udp_message->payload 	    = (char*)rt_malloc(len);
    if (!raw_udp_message->payload) {
        rt_free(raw_udp_message);
        return;
    }
    rt_memcpy(raw_udp_message->payload, data, len);
    rt_kprintf("put_cloud_downlink_message, payload_length %d\r\n", len);
    rt_err_t ret = put_cloud_downlink_message(raw_udp_message);
    if(ret != RT_EOK){
        LOG_CLOUD("put_cloud_downlink_message failed.\r\n");
        rt_free(raw_udp_message->payload);
        rt_free(raw_udp_message);
    }
}

// URC开始接收数据：+QIURC: "recv"
static void urc_recv_data_handler(struct at_client *client, char *data, rt_size_t size) {
    if (!data || size == 0) {
        return;
    }
    // return data: +QIURC: "recv",2
    rt_kprintf("\r\n+QIURC:'recv' urc_recv_data_handler: %d.\r\n", size);
    //print_hex_string(data, size);
    // 缓存模式下执行：AT+QIRD， 直吐模式下不需要主动读数据：以下逻辑无需执行    
    at_command_process_t **mqtt_read_data_commands = ec20_get_mqtt_read_data_commands();
    if (!mqtt_read_data_commands) {
        return;
    }

    // 发送MQTT接收数据指令：AT+QIRD:
    lte_command_proccess(mqtt_read_data_commands);
}

// URC接收回调（处理原始数据并转发给业务层）:+QIRD
static void urc_rd_data_handler(struct at_client *client, char *data, rt_size_t size) {
    rt_mutex_take(mqtt_receive_mutex, RT_WAITING_FOREVER);

    rt_kprintf("+QIRD urc_rd_data_handler, received size = %d\r\n", size);

    if(size == 2){
        rt_kprintf("no data received\r\n");
    }
    if (data == NULL || size == 0) {
        return;
    }

    // 2. 处理：新包开头 + 半包缓存 两种场景合并
    char* current_buf = data;
    uint32_t current_len = size;

    // 如果有未处理完的半包，先拼接
    if (partial_message != NULL && partial_message_length > 0) {
        // 重新分配内存，拼接新数据
        char* new_buf = (char*)rt_malloc(partial_message_length + current_len);
        if (new_buf == NULL) {
            // 内存分配失败，清空缓存避免泄漏
					  rt_kprintf("allocate new_buf memory failed\r\n");
            rt_free(partial_message);
            partial_message = NULL;
            partial_message_length = 0;
            return;
        }

        // 拷贝旧数据 + 新数据
        rt_memcpy(new_buf, partial_message, partial_message_length);
        rt_memcpy(new_buf + partial_message_length, current_buf, current_len);

        // 释放旧缓存
        rt_free(partial_message);

        // 更新为新拼接的缓存
        partial_message = new_buf;
        partial_message_length += current_len;

        // 切换为处理拼接后的缓冲区
        current_buf = partial_message;
        current_len = partial_message_length;
    }

    // 3. 循环处理：解决**连续粘包**（一包接一包）
    while (current_len >= 4) {  // 至少能读头部4字节
        // 检查包头：0xFF 0xEE
        if (current_buf[0] != '\xFF' || current_buf[1] != '\xEE') {
            // 无效流，直接丢弃所有缓存（协议同步失败）
					  rt_kprintf("package is not start 0xFF0xEE:\r\n");
					  print_hex_string(current_buf, current_len);
            break;
        }

        // 读取包长度（小端/你原有逻辑：data[3]<<8 | data[2]）
        uint16_t pkg_len = (current_buf[3] << 8) | current_buf[2];
        uint32_t total_pkg_len = 4 + pkg_len;  // 头部4字节 + 数据长度

        // 不够一整包，保存半包，退出
        if (current_len < total_pkg_len) {
					  rt_kprintf("package is not a whole message:\r\n");
					  print_hex_string(current_buf, current_len);					
            break;
        }

        // 4. 收到完整包 → 回调处理
        mqtt_recv_data_callback((void*)(current_buf + 4), pkg_len);

        // 5. 处理粘包：移动指针，继续处理下一包
        current_buf += total_pkg_len;
        current_len -= total_pkg_len;
    }

    // 6. 保存剩余半包（如果有）
    if (current_len > 0 && partial_message != NULL) {
        // 还有剩余数据，需要保留
        if (current_buf != partial_message) {
            // 数据在缓存头部，直接拷贝
            rt_memmove(partial_message, current_buf, current_len);
        }
        partial_message_length = current_len;
    } else {
        // 没有剩余数据，清空释放
        if (partial_message != NULL) {
            rt_free(partial_message);
            partial_message = NULL;
            partial_message_length = 0;
        }
    }
    rt_mutex_release(mqtt_receive_mutex);
}

// MQTT状态变化处理: +QMTSTAT
static void urc_mqtt_status_handler(struct at_client *client, const char *data, rt_size_t size){
    LOG_EC20("urc_mqtt_status_handler, MQTT connection status changed.\r\n");
    rt_kprintf("MQTT Status: %s\n", data);
    
    // 解析MQTT连接状态
    if (strstr(data, "disconnected")) {
        if (lte_info.lte_state == MQTT_STATE_CONNECTED) {
            lte_info.lte_state = MQTT_STATE_DISCONNECTED;
            mqtt_app_reconnect();
        }
    }
}

// MQTT消息接收处理: +QMTRECV
static void urc_mqtt_recv_handler(struct at_client *client, const char *data, rt_size_t size){
    LOG_EC20("urc_mqtt_recv_handler, MQTT message received.\r\n");
    rt_kprintf("MQTT Message: %s\n", data);
    
    // 调用接收回调处理数据
    mqtt_recv_data_callback(data, size);
}

// URC发送数据回调
static void urc_send_data_handler(struct at_client *client, const char *data, rt_size_t size){
    LOG_EC20("urc_send_data_handler, AT+QISEND then got return value '>'\r\n");
}

static void urc_closed_handler(struct at_client *client, const char *data, rt_size_t size){
    LOG_EC20("urc_closed_handler, MQTT connection closed.\r\n");
    
    if (lte_info.lte_state == MQTT_STATE_CONNECTED) {
        lte_info.lte_state = MQTT_STATE_DISCONNECTED;
    }
    mqtt_app_reconnect();
}

// 初始化MQTT应用
int mqtt_app_init(const char *server_ip, uint16_t server_port, const char *client_id, const char *username, const char *password) {
    if (!server_ip || server_port == 0) {
        return -1;
    }

    // 初始化互斥锁
    mqtt_app_mutex = rt_mutex_create("mqtt_app_mutex", RT_IPC_FLAG_FIFO);
    if (!mqtt_app_mutex) {
        return -2;
    }
    
    mqtt_receive_mutex = rt_mutex_create("mqtt_receive_mutex", RT_IPC_FLAG_FIFO);
    if (!mqtt_receive_mutex) {
        return -2;
    }

    mqtt_send_mutex = rt_mutex_create("mqtt_send_mutex", RT_IPC_FLAG_FIFO);
    if (!mqtt_send_mutex) {
        return -2;
    }
    // 保存服务器信息
    rt_strncpy(lte_info.server_ip, server_ip, sizeof(lte_info.server_ip) - 1);
    lte_info.server_port = server_port;

    // 初始化状态
    lte_info.lte_state = MQTT_STATE_INITED;
    lte_info.last_error = 0;

    // 设置MQTT连接参数
    ec20_set_mqtt_params(server_ip, server_port, client_id, username, password);
    
 	  rt_err_t ret = RT_EOK;
    if(uplink_message_send_thread == RT_NULL){
        uplink_message_send_thread = rt_thread_create("uplink_message_send_thread", mqtt_app_uplink_process, RT_NULL, RAW_UDP_THREAD_STACK_SIZE, RAW_UDP_THREAD_PRIORITY, RAW_UDP_THREAD_TIMESLICE);
        if (uplink_message_send_thread != RT_NULL){
            rt_thread_startup(uplink_message_send_thread);
        } else {
						LOG_CLOUD("Failed to create uplink_message_send_thread!\n");
            ret = RT_ERROR;
				}
    }
    LOG_EC20("MQTT app init success.\r\n");
    return 0;
}

// 启动MQTT连接
int mqtt_app_connect(rt_bool_t is_reconnect) {
    module_reboot();
    EC20_SimDetection();
    rt_mutex_take(mqtt_app_mutex, RT_WAITING_FOREVER);

    if (lte_info.lte_state != MQTT_STATE_INITED && lte_info.lte_state != MQTT_STATE_DISCONNECTED) {
        rt_mutex_release(mqtt_app_mutex);
        return -1;
    }
    lte_info.lte_state = MQTT_STATE_CONNECTING;
    
    // 初始化底层命令
    atResp = EC20_GetAtResp();    
    /* register network vaild*/
    if(!is_reconnect) {
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
            lte_info.lte_state = MQTT_STATE_DISCONNECTED;
            LOG_EC20("Network registration failed.\r\n");
            rt_mutex_release(mqtt_app_mutex);
            return -2;
        }
    }

    ec20_set_mqtt_commands_first(atResp);
    /* mqtt connect vaild*/
    if(lte_command_proccess(ltePort.mqtt_connect_commands.LTE_GetCommands()) == 0){
        lte_info.lte_state = MQTT_STATE_CONNECTED;
        LOG_EC20("MQTT connected to %s:%d.\r\n", lte_info.server_ip, lte_info.server_port);
    } else {
        lte_info.last_error = 0x02; // MQTT连接失败
        lte_info.lte_state = MQTT_STATE_DISCONNECTED;
        LOG_EC20("MQTT connection failed.\r\n");
        rt_mutex_release(mqtt_app_mutex);
        return -3;
    }
		
		int urc_set_result = at_set_urc_table(urc_table, sizeof(urc_table) / sizeof(urc_table[0]));
    if(urc_set_result != RT_EOK){
        LOG_EC20("Failed to set URC table.\r\n");
        return -3;
    }
    rt_mutex_release(mqtt_app_mutex);
    return 0;
}

// 发送业务数据 - 发布MQTT消息
int mqtt_app_publish(const char *topic, const char *data, uint16_t len, uint8_t qos) {
    if (!data || len == 0 || !topic) {
        return -1;
    }
    atResp = EC20_GetAtResp();
    rt_mutex_take(mqtt_app_mutex, RT_WAITING_FOREVER);
    if (lte_info.lte_state != MQTT_STATE_CONNECTED) {
        LOG_EC20("No MQTT connection, cannot publish data.\r\n");
        rt_mutex_release(mqtt_app_mutex);
        return -2;
    }
    // 调用mqtt协议层发布数据准备
    int ret = ec20_prepare_mqtt_publish_data(topic, data, len, qos);
    if (ret != 0) {
        lte_info.last_error = 0x10; // 发送失败
        lte_info.lte_state = MQTT_STATE_DISCONNECTED;
        LOG_EC20("MQTT publish failed, error: %d.\r\n", ret);
    }	 
    // 调用mqtt协议层发送数据命令
    lte_command_proccess(ltePort.mqtt_send_data_commands.LTE_GetCommands());
    rt_mutex_release(mqtt_app_mutex);
    return ret;
}

// 订阅主题
int mqtt_app_subscribe(const char *topic, uint8_t qos) {
    if (!topic) {
        return -1;
    }
    atResp = EC20_GetAtResp();
    rt_mutex_take(mqtt_app_mutex, RT_WAITING_FOREVER);
    if (lte_info.lte_state != MQTT_STATE_CONNECTED) {
        LOG_EC20("No MQTT connection, cannot subscribe topic.\r\n");
        rt_mutex_release(mqtt_app_mutex);
        return -2;
    }
    // 调用mqtt协议层订阅数据准备
    int ret = ec20_prepare_mqtt_subscribe_data(topic, qos);
    if (ret != 0) {
        lte_info.last_error = 0x11; // 订阅失败
        lte_info.lte_state = MQTT_STATE_DISCONNECTED;
        LOG_EC20("MQTT subscribe failed, error: %d.\r\n", ret);
    }	 
    // 调用mqtt协议层订阅命令
    lte_command_proccess(ltePort.mqtt_send_data_commands.LTE_GetCommands());
    rt_mutex_release(mqtt_app_mutex);
    return ret;
}

// 取消订阅
int mqtt_app_unsubscribe(const char *topic) {
    if (!topic) {
        return -1;
    }
    atResp = EC20_GetAtResp();
    rt_mutex_take(mqtt_app_mutex, RT_WAITING_FOREVER);
    if (lte_info.lte_state != MQTT_STATE_CONNECTED) {
        LOG_EC20("No MQTT connection, cannot unsubscribe topic.\r\n");
        rt_mutex_release(mqtt_app_mutex);
        return -2;
    }
    // 调用mqtt协议层取消订阅数据准备
    int ret = ec20_prepare_mqtt_unsubscribe_data(topic);
    if (ret != 0) {
        lte_info.last_error = 0x12; // 取消订阅失败
        lte_info.lte_state = MQTT_STATE_DISCONNECTED;
        LOG_EC20("MQTT unsubscribe failed, error: %d.\r\n", ret);
    }	 
    // 调用mqtt协议层取消订阅命令
    lte_command_proccess(ltePort.mqtt_send_data_commands.LTE_GetCommands());
    rt_mutex_release(mqtt_app_mutex);
    return ret;
}

// 强制重连
int mqtt_app_reconnect(void) {
    rt_mutex_take(mqtt_app_mutex, RT_WAITING_FOREVER);

    // 重置状态
    lte_info.lte_state = MQTT_STATE_DISCONNECTED;
    
    // 重新连接
    int ret = mqtt_app_connect(RT_TRUE);

    rt_mutex_release(mqtt_app_mutex);
    return ret;
}

// 释放资源
void mqtt_app_deinit(void) {
    rt_mutex_take(mqtt_app_mutex, RT_WAITING_FOREVER);

    // 释放互斥锁
    if (mqtt_app_mutex) {
        rt_mutex_delete(mqtt_app_mutex);
        mqtt_app_mutex = RT_NULL;
    }

    // 重置状态
    lte_info.lte_state = MQTT_STATE_UNINIT;
    lte_info.last_error = 0;

    LOG_EC20("MQTT app deinit success.\r\n");

    rt_mutex_release(mqtt_app_mutex);
}

static void mqtt_app_uplink_process(void* data) {
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

            rt_mutex_take(mqtt_send_mutex, RT_WAITING_FOREVER);
						uint16_t len = raw_udp_message->payload_length;
            char *buffer = (char *)rt_malloc(len + 4); 
            if (!buffer) {
                rt_kprintf("Failed to allocate memory for uplink message buffer.\r\n");
                free_raw_udp_message(raw_udp_message);
                rt_thread_mdelay(100);
                continue;
            }
            buffer[0] = 0xFF; // MQTT header start byte 1
            buffer[1] = 0xEE; // MQTT header start byte 2
            memcpy(buffer + 2, &len, 2); // MQTT header length bytes
            memcpy(buffer + 4, raw_udp_message->payload, len); // MQTT payload

            // 发送消息到默认主题
            mqtt_app_publish("device/uplink", buffer, len + 4, 1);

            rt_free(buffer);
            free_raw_udp_message(raw_udp_message);
            rt_mutex_release(mqtt_send_mutex);
        } 
        rt_thread_mdelay(100);
    }
}   

// 辅助函数：打印响应内容
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

// 获取LTE信息
void get_lte_info(lte_info_t* info){
    if(info != RT_NULL){
        rt_memcpy(info, &lte_info, sizeof(lte_info_t));
    }
}

// 内部函数：处理AT命令
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
        at_resp_set_info(atResp, DEFAULT_AT_RESPONE_BUFFER_SIZE, DEFAULT_AT_RESPONE_LINE_NUMBER, (*pCommands)->command_array[com].timeout);
        strLength = rt_strlen((*pCommands)->command_array[com].at_cmd);
        //组合命令参数
        if ((*pCommands)->command_array[com].get_parameters != RT_NULL) //命令存在参数
        {
          param = (*pCommands)->command_array[com].get_parameters();
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
        rt_strncpy(sendBuffer, (*pCommands)->command_array[com].at_cmd, rt_strlen((*pCommands)->command_array[com].at_cmd) + 1);
        if (param) //命令存在参数
        {
          strcat(sendBuffer, (*pCommands)->command_array[com].get_parameters());
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
      if ((*pCommands)->command_array[com].expect_response != RT_NULL && at_resp_get_line_by_kw(atResp, (*pCommands)->command_array[com].expect_response) == RT_NULL) //收不到指定返回
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
      if ((*pCommands)->command_array[com].command_handle != RT_NULL && (*pCommands)->command_array[com].command_handle() != AT_COMMAND_HANDLE_STATE_SUCCESS)
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
/*static int lte_command_proccess(at_command_process_t **pCommands)
{
    at_command_process_t *p = *pCommands;
    int result = 0;
    int retry = 0;

    while (p)
    {
        for (int i = 0; i < p->commandSize; i++)
        {
            retry = 0;
            do
            {
                result = at_exec_cmd(atResp, p->command_array[i].at_cmd, p->command_array[i].timeout);
                if (result == 0)
                {
                    if (p->command_array[i].command_handle)
                    {
                        if (p->command_array[i].command_handle() != AT_COMMAND_HANDLE_STATE_SUCCESS)
                        {
                            result = -1;
                        }
                    }
                    break;
                }
                retry++;
            } while (retry < AT_COMMAND_RETRY_MAX_COUNT);
            
            if(result != 0)
            {
                LOG_EC20("lte command exec failed, cmd: %s.\r\n", p->command_array[i].at_cmd);
                return result;
            }
        }
        p = p->next;
    }
    return result;
}*/