#include "modbus_manager.h"
#include <rtthread.h>
#include "rs485.h"
#include "modbus_parser.h"
#include <string.h>
#include <ctype.h>
#include "../utils/byte_utils.h"

static rt_mutex_t rs485_mutex       = RT_NULL;
static rt_sem_t   rs485_reply_sem   = RT_NULL;
static modbus_response_message_t    modbus_response_message;
static int       modbus_parse_result = 0;

void modbus_message_init(void)
{   
    //rt_memset(&modbus_request_message, 0, sizeof(modbus_request_message_t));
    rt_memset(&modbus_response_message, 0, sizeof(modbus_response_message_t)); 
    memset(modbus_response_message.value, 0, sizeof(modbus_response_message.value));
}

int modbus_manager_init(void){
    // 创建互斥锁（用于保护 RS485 总线访问）
    rs485_mutex = rt_mutex_create("rs485_mutex", RT_IPC_FLAG_FIFO);
    if (rs485_mutex == RT_NULL)
    {
        rt_kprintf("RS485 mutex create failed!\n");
        return -RT_ERROR;
    }

    // 创建信号量（用于等待 RS485 回复）
    rs485_reply_sem = rt_sem_create("rs485_reply_sem", 0, RT_IPC_FLAG_FIFO);
    if (rs485_reply_sem == RT_NULL)
    {
        rt_kprintf("RS485 reply semaphore create failed!\n");
			  rt_mutex_delete(rs485_mutex); // 回滚创建失败的锁
        rs485_mutex = RT_NULL;
        return -RT_ERROR;
    }
		modbus_message_init();
    return RT_EOK;
}
rt_err_t modbus_request(uint8_t *data, uint16_t len, modbus_request_message_t *modbus_request_message, modbus_response_message_t *response_message){

    uint8_t tx_buf[32] = {0};
    uint16_t tx_len = 0;
    rt_err_t ret = RT_EOK;
    rt_err_t mutex_ret = RT_EOK;
    int retry = 0;

    ret = hex_str_to_bytes(data, tx_buf, &tx_len);
    if (ret != RT_EOK)
    {
        rt_kprintf("[RS485] send err: hex convert failed(ret=%d)\n", ret);
        return ret;
    }

    modbus_request_parse(tx_buf, tx_len, modbus_request_message);

    ret = rt_mutex_take(rs485_mutex, RT_WAITING_FOREVER);
    if (ret != RT_EOK)
    {
        rt_kprintf("modbus_request: modbus_mutex_take failed!\n");
        return -RT_ERROR;
    }

    rt_sem_trytake(rs485_reply_sem);

    for (retry = 0; retry < MODBUS_MAX_RETRY; retry++)
    {
        ret = rs485_send(tx_buf, tx_len);
        if (ret <= 2)
        {
            rt_kprintf("modbus_request: modbus_send failed!\n");
            mutex_ret = rt_mutex_release(rs485_mutex);
            return -RT_ERROR;
        }

        rt_thread_mdelay(MODBUS_TURNAROUND_DELAY_MS);

        ret = rt_sem_take(rs485_reply_sem, MODBUS_RESP_TIMEOUT_MS);
        if (ret == RT_EOK)
        {
            if (modbus_parse_result == -6)
            {
                rt_kprintf("modbus_request: slave exception, fc=0x%02X, exc_code=%d\n",
                           modbus_response_message.function_code,
                           modbus_response_message.byte_number);
                mutex_ret = rt_mutex_release(rs485_mutex);
                return MODBUS_ERR_EXCEPTION;
            }
            if (modbus_parse_result < 0)
            {
                rt_kprintf("modbus_request: parse error %d, retry %d/%d\n",
                           modbus_parse_result, retry + 1, MODBUS_MAX_RETRY);
                continue;
            }
            break;
        }

        rt_kprintf("modbus_request: retry %d/%d, sem_take failed(ret=%d)!\n",
                   retry + 1, MODBUS_MAX_RETRY, ret);
    }

    mutex_ret = rt_mutex_release(rs485_mutex);
    if (mutex_ret != RT_EOK)
    {
        rt_kprintf("modbus_request: release rs485_mutex failed(ret=%d)!\n", mutex_ret);
    }

    if (ret != RT_EOK)
    {
        rt_kprintf("modbus_request: all %d retries failed!\n", MODBUS_MAX_RETRY);
        return -RT_ERROR;
    }

    response_message->slave_address = modbus_response_message.slave_address;
    response_message->function_code = modbus_response_message.function_code;
    response_message->byte_number   = modbus_response_message.byte_number;
    response_message->crc           = modbus_response_message.crc;

    if (modbus_response_message.value != NULL && modbus_response_message.byte_number > 0)
    {
        memcpy(response_message->value, modbus_response_message.value, modbus_response_message.byte_number);
    }
    else
    {
        memset(response_message->value, 0, sizeof(response_message->value));
    }

    return ret;
}
rt_err_t modbus_request_setting(uint8_t *data, uint16_t len, uint8_t *setting_value, modbus_request_message_t *modbus_request_message,  modbus_response_message_t *response_message){
    modbus_request_parse(data, len, modbus_request_message);
    //update data with setting_value, then send to modbus slave
    //modbus_request_message.setting_value = setting_value;
    return rs485_send(data, len);
};


rt_err_t rs485_response_callback(uint8_t *data, uint16_t len){
    modbus_parse_result = modbus_response_parse(data, len, &modbus_response_message);
    if (modbus_parse_result < 0)
    {
        rt_kprintf("rs485_response_callback: parse failed, err=%d\n", modbus_parse_result);
    }
    return rt_sem_release(rs485_reply_sem);
};

INIT_COMPONENT_EXPORT(modbus_manager_init);