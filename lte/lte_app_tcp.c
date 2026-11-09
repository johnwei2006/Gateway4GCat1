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
#include <rtdef.h>
#include <rthw.h>

// 应用层私有数据结构

static at_response_t atResp;

static rt_mutex_t   tcp_connect_mutex;          // connect mutex
static rt_mutex_t   tcp_receive_mutex;      // receive tcp mutex
static rt_mutex_t   tcp_send_mutex;         // send tcp mutex
static rt_mutex_t   lte_command_mutex = RT_NULL;
static volatile uint8_t urc_processing_flag = 0;  // URC处理中标志，防止重入

static lte_info_t   lte_info = {0};         // 静态实例
static char*        partial_message = RT_NULL;
static uint8_t      partial_message_length=0;
rt_thread_t uplink_message_send_thread = RT_NULL;

lte_port_t ltePort = {
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
    .check_connect_commands = {
        .LTE_GetCommands        = ec20_get_udp_check_connect_commands,
        .LTE_SetCommandsFirst   = ec20_set_udp_commands_first,
    },
    .httpCommands = {
        .LTE_GetCommands        = EC20_GetHttpCommands,
        .LTE_SetCommandsFirst   = EC20_SetHttpCommandsFirst,
    }
};

static struct at_urc urc_table[] = {
 //   {">",                          "\r\n",        urc_send_data_handler},
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
  ltePort.check_connect_commands.LTE_SetCommandsFirst(atResp);
  ltePort.httpCommands.LTE_SetCommandsFirst(atResp);
  //networkState = NETWORK_STATE_NOT_OK;
  LOG_EC20("lte module rebooted.\r\n");
}

// 处理UDP接收数据回调 (两种接收方式之一：1、udp_recv_data_callback；2、urc_rd_data_handler)
 /*static void udp_recv_data_callback(const char *data, uint16_t len){
    if (data == NULL || len == 0 || len > 2048) {
        rt_kprintf("Warning: udp_recv_data_callback invalid params, data=%p, len=%d\r\n", data, len);
        return;
    }    

    raw_udp_message_t *raw_udp_message = (raw_udp_message_t *)rt_malloc(sizeof(raw_udp_message_t));
    if (!raw_udp_message) {
        rt_kprintf("Error: Failed to allocate raw_udp_message\r\n");
        return;
    }
    // 填充UDP消息结构体
    raw_udp_message->payload 		    = (char*)rt_malloc(len);
    if (!raw_udp_message->payload) {
        rt_kprintf("Error: Failed to allocate payload buffer, len=%d\r\n", len);
        rt_free(raw_udp_message);
        return;
    }    
    raw_udp_message->payload_length = len;        
    rt_memcpy(raw_udp_message->payload, data, len);
    rt_kprintf("put_cloud_downlink_message, payload_length %d\r\n", len);
	//  print_hex_string(raw_udp_message->payload , raw_udp_message->payload_length);    
    rt_err_t ret = put_cloud_downlink_message(raw_udp_message);
    if(ret != RT_EOK){
        LOG_CLOUD("put_cloud_downlink_message failed.\r\n");
        rt_free(raw_udp_message->payload);
        rt_free(raw_udp_message);
        return;        
    }
 }*/

// urc开始接收数据：+QIURC: "recv"
// 1、+QIURC: "recv" 触发时，发送UDP接收数据指令：QIRD:
// 2、+QIRD: 触发时，解析数据并调用udp_recv_data_callback或urc_recv_data_handler处理
// cache mode
static void urc_recv_data_handler(struct at_client *client, char *data, rt_size_t size) {
    if (!data || size == 0) {
        return;
    }
    // 检查是否正在处理URC，防止重入
    if (urc_processing_flag) {
        rt_kprintf("Warning: urc_recv_data_handler: URC already processing, ignoring\r\n");
        return;
    }
    urc_processing_flag = 1;    
    // 获取接收互斥锁，防止与其他发送/接收操作冲突
    rt_err_t lock_result = rt_mutex_take(tcp_receive_mutex, 100);
    if (lock_result != RT_EOK) {
        rt_kprintf("Warning: urc_recv_data_handler: Failed to take tcp_receive_mutex\r\n");
        urc_processing_flag = 0;
        return;
    }    
    // return data: +QIURC: "recv",2
    rt_kprintf("\r\n+QIURC:'recv' urc_recv_data_handler: %d.\r\n", size);
    if (size < 10) {  // "+QIURC: \"recv\"" 至少10个字符
        rt_kprintf("Warning: Invalid QIURC recv notification, size too small\r\n");
        rt_mutex_release(tcp_receive_mutex);
        urc_processing_flag = 0;
        return;
    }    
    //print_hex_string(data, size);
    // 缓存模式下执行：AT+QIRD， 直吐模式下不需要主动读数据：以下逻辑无需执行    
    at_command_process_t **udp_read_data_commands = ec20_get_udp_read_data_commands();
    if (!udp_read_data_commands) {
        rt_kprintf("Warning: udp_read_data_commands is NULL\r\n");
        rt_mutex_release(tcp_receive_mutex);
        urc_processing_flag = 0;
        return;
    }

    // 发送UDP接收数据指令：AT+QIRD:
    rt_kprintf("urc_recv_data_handler: sending AT+QIRD command\r\n");
    rt_err_t result = lte_command_proccess(udp_read_data_commands);
    rt_kprintf("urc_recv_data_handler: AT+QIRD result=%d\r\n", result);

    rt_mutex_release(tcp_receive_mutex);
    urc_processing_flag = 0;
    if(result != RT_EOK){
        rt_kprintf("Warning: lte_command_proccess failed, result=%d\r\n", result);
        return;
    }
 }

 // urc接收回调（处理原始数据并转发给业务层）:+QIRD: (两种接收方式之一：1、udp_recv_data_callback；2、urc_rd_data_handler)
static void urc_rd_data_handler(struct at_client *client, char *data, rt_size_t size) {
    if (urc_processing_flag) {
        rt_kprintf("Warning: urc_rd_data_handler: URC already processing, ignoring\r\n");
        return;
    }
    urc_processing_flag = 1;
        
    rt_err_t lock_result = rt_mutex_take(tcp_receive_mutex, 100);
    if (lock_result != RT_EOK) {
        rt_kprintf("Warning: Failed to take tcp_receive_mutex\r\n");
        urc_processing_flag = 0;
        return;
    }
    rt_kprintf("+QIRD urc_rd_data_handler, received size = %d\r\n", size);

    if (data == NULL || size < 4) {
        rt_kprintf("Warning: Invalid data received, size = %d\r\n", size);
        if (size == 2) {
            rt_kprintf("Ignore 2-byte response (likely AT response terminator)\r\n");
        }
        goto exit;
    }

    char* current_buf = data;
    uint32_t current_len = size;
    rt_bool_t using_partial = RT_FALSE;

    if (partial_message != NULL && partial_message_length > 0) {
        char* new_buf = (char*)rt_malloc(partial_message_length + current_len);
        if (new_buf == NULL) {
            rt_kprintf("allocate new_buf memory failed\r\n");
            rt_free(partial_message);
            partial_message = NULL;
            partial_message_length = 0;
            goto exit;  
        }
        rt_memcpy(new_buf, partial_message, partial_message_length);
        rt_memcpy(new_buf + partial_message_length, current_buf, current_len);
        rt_free(partial_message);
        partial_message = new_buf;
        partial_message_length += current_len;
        current_buf = partial_message;
        current_len = partial_message_length;
        using_partial = RT_TRUE;
    }

    while (current_len >= 4) {
        while (current_len >= 2 && current_buf[0] == 0x0D && current_buf[1] == 0x0A) {
            current_buf += 2;
            current_len -= 2;
            rt_kprintf("DEBUG: Skipped \\r\\n prefix, remaining: %d\r\n", current_len);
        }
        while (current_len >= 1 && current_buf[0] == 0x0A) {
            current_buf += 1;
            current_len -= 1;
            rt_kprintf("DEBUG: Skipped 0x0A prefix, remaining: %d\r\n", current_len);
        }
        
        if (current_len < 4) {
            rt_kprintf("DEBUG: Not enough data after skipping prefix\r\n");
            break;
        }        
        if (current_buf[0] != '\xFF' || current_buf[1] != '\xEE') {
            rt_kprintf("Warning: Invalid package header, skipping byte\r\n");
            current_buf += 1;
            current_len -= 1;
            continue;
        }

        uint16_t pkg_len = (current_buf[3] << 8) | current_buf[2];
        uint32_t total_pkg_len = 4 + pkg_len;

        if (pkg_len > 2048) {
            rt_kprintf("Warning: Invalid package length %d, likely corrupted data\r\n", pkg_len);
            break;
        }
        if (current_len < total_pkg_len) {
            rt_kprintf("package is not a whole message:\r\n");
            print_hex_string(current_buf, current_len);					
            break;
        }

        //rt_kprintf("DEBUG: Processing package, pkg_len=%d, total=%d, remaining=%d\r\n", pkg_len, total_pkg_len, current_len);
        
        raw_udp_message_t *raw_udp_message = (raw_udp_message_t*)rt_malloc(sizeof(raw_udp_message_t));
        if (raw_udp_message == RT_NULL) {
            rt_kprintf("Error: alloc raw_udp_message failed\r\n");
            rt_free(raw_udp_message);
            break;
        }
        raw_udp_message->payload = (char*)rt_malloc(pkg_len);
        if (raw_udp_message->payload == RT_NULL) {
            rt_free(raw_udp_message);
            rt_kprintf("Error: alloc payload failed\r\n");
            break;
        }
        rt_memcpy(raw_udp_message->payload, current_buf + 4, pkg_len);
        raw_udp_message->payload_length = pkg_len;
        put_cloud_downlink_message(raw_udp_message);
        rt_kprintf("put_cloud_downlink_message, payload_length %d\r\n", pkg_len);

        current_buf += total_pkg_len;
        current_len -= total_pkg_len;
        //rt_kprintf("DEBUG: Remaining data after processing: %d\r\n", current_len);

        if (using_partial && current_len == 0) {
            rt_free(partial_message);
            partial_message = RT_NULL;
            partial_message_length = 0;
            using_partial = RT_FALSE;
        }
     }

     if (current_len > 0 && current_len < 4) {
        if ((current_len == 2 && current_buf[0] == 0x0D && current_buf[1] == 0x0A) ||
            (current_len == 1 && current_buf[0] == 0x0A)) {
            //rt_kprintf("DEBUG: Remaining is \\r\\n or 0x0A, clearing partial_message\r\n");
            if (partial_message != RT_NULL) {
                rt_free(partial_message);
                partial_message = RT_NULL;
                partial_message_length = 0;
            }
        } else {
            //rt_kprintf("DEBUG: Saving partial message, len=%d\r\n", current_len);
            char* new_buf = (char*)rt_malloc(current_len);
            if (new_buf != RT_NULL) {
                if (partial_message != RT_NULL) {
                    rt_free(partial_message);
                }
                rt_memcpy(new_buf, current_buf, current_len);
                partial_message = new_buf;
                partial_message_length = current_len;
            }
        }
    } else if (current_len >= 4) {
        rt_kprintf("Warning: Remaining data >= 4 bytes after processing, possible data issue\r\n");
        print_hex_string(current_buf, current_len > 16 ? 16 : current_len);
        if (partial_message != RT_NULL) {
            rt_free(partial_message);
            partial_message = RT_NULL;
            partial_message_length = 0;
        }
    } else {
    // 如果剩余数据是\r\n或0x0A等控制字符，不保存为半包
    if (current_len == 2 && current_buf[0] == 0x0D && current_buf[1] == 0x0A) {
        //rt_kprintf("DEBUG: Remaining data is \\r\\n, clearing partial_message\r\n");
        if (partial_message != RT_NULL) {
            rt_free(partial_message);
            partial_message = RT_NULL;
            partial_message_length = 0;
        }
    } else if (current_len == 1 && current_buf[0] == 0x0A) {
        //rt_kprintf("DEBUG: Remaining data is 0x0A, clearing partial_message\r\n");
        if (partial_message != RT_NULL) {
            rt_free(partial_message);
            partial_message = RT_NULL;
            partial_message_length = 0;
        }
    } else if (partial_message != RT_NULL) {
        // 如果是有效数据才保存
        rt_free(partial_message);
        partial_message = RT_NULL;
        partial_message_length = 0;
    }
    }
exit:
    urc_processing_flag = 0;
    rt_mutex_release(tcp_receive_mutex);
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
    //udp_app_reconnect();
    rt_hw_cpu_reset();
 }

// 初始化UDP应用
int udp_app_init(const char *server_ip, uint16_t server_port) {
    if (!server_ip || server_port == 0) {
        return -1;
    }

    // 初始化互斥锁
    tcp_connect_mutex = rt_mutex_create("tcp_connect_mutex", RT_IPC_FLAG_FIFO);
    if (!tcp_connect_mutex) {
        return -2;
    }
    
    tcp_receive_mutex = rt_mutex_create("tcp_receive_mutex", RT_IPC_FLAG_FIFO);
    if (!tcp_receive_mutex) {
        return -2;
    }

    tcp_send_mutex = rt_mutex_create("tcp_send_mutex", RT_IPC_FLAG_FIFO);
    if (!tcp_send_mutex) {
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
    rt_mutex_take(tcp_connect_mutex, RT_WAITING_FOREVER);

    if (lte_info.lte_state != UDP_STATE_INITED && lte_info.lte_state != UDP_STATE_DISCONNECTED) {
        rt_mutex_release(tcp_connect_mutex);
        return -1;
    }
    lte_info.lte_state = UDP_STATE_CONNECTING;
    
    // 设置服务器信息并启动连接流程
    char *server_ip_ptr = lte_info.server_ip;
    ec20_set_udp_server(&server_ip_ptr, lte_info.server_port);
    
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
            lte_info.lte_state = UDP_STATE_DISCONNECTED;
            LOG_EC20("Network registration failed.\r\n");
            rt_mutex_release(tcp_connect_mutex);
            return -2;
        }
    }

    ec20_set_udp_commands_first(atResp);
    /* udp connect vaild*/
    if(lte_command_proccess(ltePort.udp_connect_commands.LTE_GetCommands()) == 0){
        lte_info.lte_state = UDP_STATE_CONNECTED;
        if(connect_callback != NULL){
            connect_callback(UDP_STATE_CONNECTED);
        }
        LOG_EC20("UDP connected to %s:%d.\r\n", lte_info.server_ip, lte_info.server_port);
    } else {
        lte_info.last_error = 0x02; // UDP连接失败
        lte_info.lte_state = UDP_STATE_DISCONNECTED;
        if(connect_callback != NULL){
            connect_callback(UDP_STATE_DISCONNECTED);
        }
        LOG_EC20("UDP connection failed.\r\n");
        rt_mutex_release(tcp_connect_mutex);
        return -3;
    }
		
		int urc_set_result = at_set_urc_table(urc_table, sizeof(urc_table) / sizeof(urc_table[0]));
    if(urc_set_result != RT_EOK){
        LOG_EC20("Failed to set URC table.\r\n");
        rt_mutex_release(tcp_connect_mutex);
        return -3;
    }
    rt_mutex_release(tcp_connect_mutex);
    return 0;
}

// 发送业务数据
int udp_app_send(const char *data, uint16_t len) {
    if (!data || len == 0) {
        return -1;
    }
    atResp = EC20_GetAtResp();
    if (lte_info.lte_state != UDP_STATE_CONNECTED) {
        LOG_EC20("No UDP connection, cannot send data.\r\n");
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
    ret = lte_command_proccess(ltePort.udp_send_data_commands.LTE_GetCommands());
    return ret;
}

// 强制重连
int udp_app_reconnect(void) {
    rt_mutex_take(tcp_connect_mutex, RT_WAITING_FOREVER);

    // 重置状态
    lte_info.lte_state = UDP_STATE_DISCONNECTED;
    
    // 重新连接
    int ret = udp_app_connect(RT_TRUE, NULL);

    rt_mutex_release(tcp_connect_mutex);
    return ret;
}

// 释放资源
void udp_app_deinit(void) {
    rt_mutex_take(tcp_connect_mutex, RT_WAITING_FOREVER);

    // 释放互斥锁
    if (tcp_connect_mutex) {
        rt_mutex_delete(tcp_connect_mutex);
        tcp_connect_mutex = RT_NULL;
    }

    // 重置状态
    lte_info.lte_state = UDP_STATE_UNINIT;
    lte_info.last_error = 0;

    LOG_EC20("UDP app deinit success.\r\n");

    rt_mutex_release(tcp_connect_mutex);
}

static void udp_app_uplink_process(void* data) {
    // This function should implement the logic to process uplink messages
    while (1) {
        
        raw_udp_message_t *raw_udp_message = RT_NULL;
        // 从队列中获取消息
        if (get_cloud_uplink_message(&raw_udp_message) == RT_EOK) {
            if(raw_udp_message == RT_NULL) {
                rt_thread_mdelay(10);
                continue;
            }

            rt_mutex_take(tcp_send_mutex, RT_WAITING_FOREVER);
						uint16_t len = raw_udp_message->payload_length;
            char *buffer = (char *)rt_malloc(len + 4); 
            if (!buffer) {
                rt_kprintf("Failed to allocate memory for uplink message buffer.\r\n");
                free_raw_udp_message(raw_udp_message);
                rt_mutex_release(tcp_send_mutex); 
                rt_thread_mdelay(10);
                continue;
            }
            buffer[0] = 0xFF; // TCP header start byte 1
            buffer[1] = 0xEE; // TCP header start byte 2
            memcpy(buffer + 2, &len, 2); // TCP header length bytes
            memcpy(buffer + 4, raw_udp_message->payload, len); // TCP payload

            // 发送消息
            rt_kprintf("Sending uplink message, payload length: %d.\r\n", len + 4);
            int send_result = udp_app_send(buffer, len + 4);
            
            if (send_result != 0) {
                rt_kprintf("Warning: Failed to send uplink message, result: %d, putting back to queue\n", send_result);
                // 尝试将消息放回队列以供后续重试，如果放回失败则丢弃
                if (put_cloud_uplink_message(&raw_udp_message) != RT_EOK) {
                    rt_kprintf("Error: Failed to requeue message, discarding\n");
                    rt_free(buffer);
                    free_raw_udp_message(raw_udp_message);
                } else {
                    rt_kprintf("Message requeued successfully for retry\n");
                    rt_free(buffer);
                }
            } else {
                // 发送成功，正常释放资源
                rt_free(buffer);
                free_raw_udp_message(raw_udp_message);
            }
            rt_mutex_release(tcp_send_mutex);

            //lte_command_proccess(ltePort.check_connect_commands.LTE_GetCommands());
        } 
        rt_thread_mdelay(10);
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
        //rt_mutex_take(udp_app_mutex, RT_WAITING_FOREVER);
        rt_memcpy(info, &lte_info, sizeof(lte_info_t));
        //rt_mutex_release(udp_app_mutex);
    }
}

void set_lte_status(lte_state_t lte_state){
    lte_info.lte_state = lte_state;
}

int lte_command_proccess(at_command_process_t **command_array_list)
{
    char *sendBuffer = RT_NULL;
    uint8_t paramInit = 0;
    if (lte_command_mutex == RT_NULL) {
        lte_command_mutex = rt_mutex_create("lte_cmd_mutex", RT_IPC_FLAG_FIFO);
        if (lte_command_mutex == RT_NULL) {
            rt_kprintf("Error: Failed to create lte_command_mutex\r\n");
            return -1;
        }
    }
    
    // ★ 添加：获取全局锁，防止并发调用
    rt_err_t lock_result = rt_mutex_take(lte_command_mutex, RT_WAITING_FOREVER);
    if (lock_result != RT_EOK) {
        rt_kprintf("Warning: Failed to take lte_command_mutex\r\n");
        return -2;
    }
        
	atResp = EC20_GetAtResp();
  if (atResp == NULL)
  {
    rt_kprintf("lte_app.lte_command_proccess: LTE atResp is NULL.\r\n");
    rt_mutex_release(lte_command_mutex);
    return -1;
  }
  if ((*command_array_list) == NULL) // allow command array list to be NULL
  {
    rt_mutex_release(lte_command_mutex);
    return -2;
  }
  do
  {
    int tickMs         = 0;
    int at_retry_count = 0;                                      // retry count

    for (uint8_t i = 0; i < (*command_array_list)->commandSize;) //command group process
    {
      if (paramInit == 0) 
      {
        char *param   = RT_NULL;
        int strLength = 0;
        paramInit     = 1;
        //set command response info
        at_resp_set_info(atResp, DEFAULT_AT_RESPONE_BUFFER_SIZE, DEFAULT_AT_RESPONE_LINE_NUMBER, (*command_array_list)->command_array[i].timeout);
        strLength = rt_strlen((*command_array_list)->command_array[i].at_cmd);
        //command parameter handle
        if ((*command_array_list)->command_array[i].get_parameters != RT_NULL) // command has parameters
        {
          param = (*command_array_list)->command_array[i].get_parameters();
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
          i = 0;
          at_retry_count = 0;
          paramInit      = 0;
          EC20_Reboot();
          udp_app_reconnect();
          rt_mutex_release(lte_command_mutex);
          return -3;
        }
        rt_strncpy(sendBuffer, (*command_array_list)->command_array[i].at_cmd, rt_strlen((*command_array_list)->command_array[i].at_cmd) + 1);
        if (param) // command has parameters
        {
          strcat(sendBuffer, (*command_array_list)->command_array[i].get_parameters());
        }
        tickMs = rt_tick_get_millisecond(); // get current tick count
      }
      rt_kprintf("lte_app.lte_command_proccess to send   : %s \r\n", sendBuffer);
      //发送命令
      at_exec_cmd(atResp, sendBuffer);

      rt_kprintf("\natResp with at execution---------------------------------------------\n");
      printatresp(atResp->buf,atResp->buf_len);
			
      //打印命令  
      rt_kprintf("lte_app.lte_command_proccess to receive: %d/%d lines\r\n", atResp->line_counts, atResp->line_num);
      for (int k = 1; k <= atResp->line_counts; k++)
      {
        rt_kprintf("   line %d: %s\r\n", k, at_resp_get_line(atResp, k));
      }
      if(atResp->buf_len == 0 || atResp->line_counts > 0){
        if ((*command_array_list)->command_array[i].expect_response != RT_NULL && at_resp_get_line_by_kw(atResp, (*command_array_list)->command_array[i].expect_response) == RT_NULL) //收不到指定返回
        {
            if (++at_retry_count >= AT_COMMAND_RETRY_MAX_COUNT) // retry count exceed max retry count
            {
            i = 0;
            at_retry_count     = 0;
            paramInit          = 0;
            lte_info.lte_state = UDP_STATE_UNINIT;
            rt_kprintf("lte_app.lte_command_proccess: at cmd %s max retry, system reboot!!!!!!!!!!!!\r\n", sendBuffer);
            //EC20_Reboot();
            //udp_app_reconnect();
            rt_thread_mdelay(5*1000);
            rt_hw_cpu_reset();
            return -2;
            }
            continue;
        }
      }
      //command handle
      if ((*command_array_list)->command_array[i].command_handle != RT_NULL && (*command_array_list)->command_array[i].command_handle() != AT_COMMAND_HANDLE_STATE_SUCCESS)
      {
        if (rt_tick_get_millisecond() - tickMs > (int)(*command_array_list)->waitSecond * 1000) //command timeout process
        {
          i = 0;
          at_retry_count = 0;
          paramInit      = 0;
          rt_kprintf("lte_app.lte_command_proccess: at cmd %s timeout, system reboot!!!!!!!!!!!!\r\n", sendBuffer);
          //EC20_Reboot();
          rt_thread_mdelay(5*1000);
          rt_hw_cpu_reset();
          return -3;
        }
        continue;
      }		
			rt_memset(atResp->buf,0,atResp->buf_len);
      i++; //execute next command
      at_retry_count = 0;
      paramInit = 0;
      if (sendBuffer != RT_NULL)
      {
        rt_free(sendBuffer);
        sendBuffer = RT_NULL;
      }
    }

    if ((*command_array_list)->ok_function != RT_NULL)
      (*command_array_list)->ok_function(); //execute success callback

    (*command_array_list) = (*command_array_list)->next; //execute next command group

  } while ((*command_array_list) != RT_NULL);
  LOG_CLOUD("lte_app.lte_command_proccess: all commands processed successfully.\r\n");
  rt_mutex_release(lte_command_mutex);
  return 0;
}