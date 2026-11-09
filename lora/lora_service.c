#include "lora_service.h"
#include <rtthread.h>
#include <string.h>
#include "../transport/transport_message_queue.h"
static lora_service_config_t lora_config;
static struct rt_mutex lora_service_mutex;
static struct rt_thread lora_tx_thread;
static char rx_thread_stack[512];
static lora_state_t lora_state          = LORA_STATE_IDLE;//LORA_STATE_TX_DONE;
static rt_bool_t channel_ready          = RT_TRUE;
static uint8_t   cad_tried_times        = 0;
static lora_message_t *lora_tx_message  = RT_NULL;

// 接收线程函数
static void lora_tx_thread_entry(void *parameter)
{
    
    rt_err_t ret =RT_EOK;
    
    while (1)
    {
        switch(lora_state)
        {
            case LORA_STATE_INIT:
                lora_state = LORA_STATE_CONFIG;
                break;
            case LORA_STATE_CONFIG:
                lora_state = LORA_STATE_START_RX;
                break;
            case LORA_STATE_TX_DONE:
                /* used for test
                lora_message_t msg;
                uint8_t payload[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
                msg.payload = rt_malloc(8);
                memcpy(msg.payload, payload, 8);
                msg.payload_length = sizeof(payload);
                put_lora_downlink_message(&msg);
                */
                lora_state = LORA_STATE_IDLE;
                break;
            case LORA_STATE_IDLE:
                if (lora_tx_message != RT_NULL)
                {
                    ret = RT_EOK; // Ready to send existing message(send failed message)
                } else {
                    // Get new message if none exists
								//ret = rt_sem_take(lora_tx_sem, RT_WAITING_FOREVER);				
					   //if (ret == RT_EOK){
							   ret = get_lora_downlink_message(&lora_tx_message);;
							   							 
     				 //}
                    
                }
                if (ret == RT_EOK)
                {
                    //lora_state = LORA_STATE_START_TX;
                    //ret = lora_driver_send(lora_tx_message->payload, lora_tx_message->payload_length, 1000);
                    lora_state = LORA_STATE_START_CAD;
                    lora_driver_cad(0);
                } else {
										lora_state = LORA_STATE_TX_DONE;
								}
                break;
            case LORA_STATE_CAD_DONE:
                if (channel_ready)
                {
                    cad_tried_times = 0;
                    // 发送消息
                    ret = lora_driver_send(lora_tx_message->payload, lora_tx_message->payload_length, 1000);
                    if (ret != RT_EOK)
                    {
                        rt_kprintf("LoRaService: Failed to send message, ret = %d\n", ret);
                    }
                    rt_free(lora_tx_message->payload);
                    lora_tx_message->payload = RT_NULL;
                    rt_free(lora_tx_message);
                    lora_tx_message = RT_NULL;
                }
                if(cad_tried_times > 5){
                    cad_tried_times = 0;
                    lora_power_pin_init();
                }
                lora_state = LORA_STATE_IDLE;
                break;
            default:
                break;
        }
        rt_thread_mdelay(100);
    }
}

int lora_service_init(void)
{
    rt_err_t ret = RT_EOK;
    
    // 初始化互斥锁
    ret = rt_mutex_init(&lora_service_mutex, "lora_service_mutex", RT_IPC_FLAG_FIFO);
    if (ret != RT_EOK)
        return ret;
    
    lora_mod_params_t lora_params = {
        .bandwidth          = RF_BW_125KHZ, // 带宽（0表示使用默认值）
        .spreading_factor   = RF_DR_SF8,    // 扩频因子SF8
        .coding_rate        = 4,            // 编码率：4/5
        .frequency          = 475100000,    // 475.1MHz
        .tx_power           = 17,           // 17dBm
        .preamble_len       = 1025,         // 前导码长度
        .crc_en             = false,        // 关闭CRC校验（按需开启）
        .invert_iq          = false,        // 不反转IQ
        .timeout            = 3000          // 超时
    };
   
    // 初始化驱动层
    ret = lora_driver_init(&lora_params, lora_service_on_cad_done, lora_service_on_tx_done, lora_service_on_tx_timeout);
    if (ret != RT_EOK)
        return ret;
    
    // 启动接收线程
    ret = rt_thread_init(&lora_tx_thread,
                        "lora_rx_thread",
                        lora_tx_thread_entry,
                        RT_NULL,
                        rx_thread_stack,
                        sizeof(rx_thread_stack),
                        12,
                        10);
    if (ret == RT_EOK)
    {
        rt_thread_startup(&lora_tx_thread);
    }
    else
    {
        lora_driver_deinit();
        rt_mutex_detach(&lora_service_mutex);
        return ret;
    }
    
    return RT_EOK;
}

rt_err_t lora_service_deinit(void)
{
    // 停止接收线程
    rt_thread_delete(&lora_tx_thread);
    
    // 反初始化驱动
    lora_driver_deinit();
    
    // 删除互斥锁
    rt_mutex_detach(&lora_service_mutex);
    
    return RT_EOK;
}

rt_err_t lora_service_get_config(lora_service_config_t *config)
{
    if (config == NULL)
        return RT_ERROR;
    
    rt_mutex_take(&lora_service_mutex, RT_WAITING_FOREVER);
    memcpy(config, &lora_config, sizeof(lora_service_config_t));
    rt_mutex_release(&lora_service_mutex);
    
    return RT_EOK;
}

rt_err_t lora_service_update_config(const lora_service_config_t *config)
{
    if (config == NULL)
        return RT_ERROR;
    
    rt_mutex_take(&lora_service_mutex, RT_WAITING_FOREVER);
    
    // 更新可修改的配置
    lora_config.auto_ack = config->auto_ack;
    lora_config.tx_timeout = config->tx_timeout;
    lora_config.rx_timeout = config->rx_timeout;
    lora_config.max_retry = config->max_retry;
    
    // 如果调制参数有变化，更新驱动
    if (memcmp(&lora_config.mod_params, &config->mod_params, sizeof(lora_mod_params_t)) != 0)
    {
        lora_config.mod_params = config->mod_params;
        lora_driver_set_mod_params(&lora_config.mod_params);
    }
    
    rt_mutex_release(&lora_service_mutex);
    
    return RT_EOK;
}

static void lora_service_on_cad_done(bool channel_activity_detected){
    lora_state = LORA_STATE_CAD_DONE;
    // CAD完成，成功后发送报文，不成功继续等待，再次执行CAD
    cad_tried_times++;
    // 返回待机模式
    Radio.Standby();
    channel_ready = !channel_activity_detected;
    // 解锁
    /*rt_err_t ret = lora_driver_release_mutex();
    if (ret != RT_EOK)
    {
        rt_kprintf("lora_service_on_cad_done: Failed to release lora_mutex, ret = %d\n", ret);
        return;
    }*/
}
static void lora_service_on_tx_done(){
    // 发送完成，删除报文缓存
    lora_state = LORA_STATE_TX_DONE;
    lora_tx_message = RT_NULL;
}
static void lora_service_on_tx_timeout(){
    // 发送超时，报文缓存，等待重传
    lora_state = LORA_STATE_IDLE;
}

//INIT_ENV_EXPORT(lora_service_init);