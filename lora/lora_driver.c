#include "lora_driver.h"
#include <rtdevice.h>
#include <rtthread.h>
#include <lora-radio.h>
#include <rthw.h>
#include <string.h>
#include "sx126x.h"
#include "board.h"
#include "../transport_lora/lora_message.h"
// 全局变量
static lora_mod_params_t    lora_mod_params;
static struct rt_semaphore  rx_sem;
static struct rt_mutex      lora_mutex;
//static uint8_t              rx_buffer[256];  // 接收缓冲区
// Radio事件结构体
static RadioEvents_t radio_events = {
    .TxDone = NULL,
    .RxDone = NULL,
    .CadDone = NULL,
    .TxTimeout = NULL,
    .RxTimeout = NULL,
    .RxError = NULL
};


// 接收完成回调
static void on_rx_done(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    lora_rx_message_t msg;
    msg.payload         = payload;
    msg.payload_length  = size;
    msg.rssi            = rssi;
    msg.snr             = snr;
   
    put_lora_uplink_message(&msg);
}

// 接收超时回调
static void on_rx_timeout(void)
{
    rt_sem_release(&rx_sem);
	Radio.Sleep();
}

// 接收错误回调
static void on_rx_error(void)
{
    rt_sem_release(&rx_sem);
	Radio.Sleep();
}


rt_err_t lora_driver_init(const lora_mod_params_t *mod_params, on_cad_done_callback cad_done_callback, on_tx_done_callback tx_done_callback, on_tx_timeout_callback tx_timeout_callback)
{
    rt_err_t ret;

    if (mod_params == RT_NULL)
        return RT_ERROR;
    
    // 复制配置参数
    memcpy(&lora_mod_params, mod_params, sizeof(lora_mod_params_t));
    
    // 初始化互斥锁
    ret = rt_mutex_init(&lora_mutex, "lora_mutex", RT_IPC_FLAG_FIFO);
    if (ret != RT_EOK)
        return ret;
    
    // 初始化信号量
    ret = rt_sem_init(&rx_sem, "lora_rx_sem", 0, RT_IPC_FLAG_FIFO);
    if (ret != RT_EOK)
        return ret;
    
    // 配置GPIO引脚
    lora_power_pin_init();
    
    // 注册Radio事件回调
    radio_events.RxDone      = on_rx_done;
    radio_events.RxTimeout   = on_rx_timeout;
    radio_events.RxError     = on_rx_error;

    radio_events.TxDone      = tx_done_callback;
    radio_events.CadDone     = cad_done_callback;
    radio_events.TxTimeout   = tx_timeout_callback;
    
    // 初始化Radio
    if (!Radio.Init(&radio_events))     // 初始化射频模块，绑定事件回调
    {
        //rt_sprintf("lora radio Init failed!\n");
        rt_hw_cpu_reset();
        return RT_ERROR;
    }
    
    // 配置调制参数
    lora_driver_set_mod_params(mod_params);
    
    // 默认进入接收模式
    Radio.Rx(0);  // 无超时接收
    
    return RT_EOK;
}


rt_err_t lora_driver_deinit(void)
{
    // 关闭Radio
    Radio.Sleep();
   
    // 删除信号量和互斥锁
    rt_sem_detach(&rx_sem);
    rt_mutex_detach(&lora_mutex);
    
    return RT_EOK;
}

rt_err_t lora_driver_cad(uint32_t timeout_ms)
{
    rt_err_t ret = RT_EOK;

    ret = rt_mutex_take(&lora_mutex, RT_WAITING_FOREVER);
    if (ret != RT_EOK){
    
        rt_kprintf("lora_driver_cad: Failed to take lora_mutex, ret = %d\n", ret);
        return ret;
    }
    // 确保在待机模式
    Radio.Standby();
    
    // 配置CAD参数
    //Radio.SetCadParams(LORA_CAD_08_SYMBOLS, 10, 0, false);

    // 执行CAD检测
    Radio.StartCad();
    return ret;
}

rt_err_t lora_driver_send(uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    rt_err_t ret = RT_EOK;
    rt_tick_t start_tick;
    
    if (data == RT_NULL || len == 0)
        return RT_ERROR;

    ret = rt_mutex_take(&lora_mutex, RT_WAITING_FOREVER);
    if (ret != RT_EOK){
        rt_kprintf("lora_driver_send: Failed to take lora_mutex, ret = %d\n", ret);
        return ret;
    }

    // 准备发送
    Radio.Send(data, len);
    rt_kprintf("lora_driver_send: send data len = %d\n", len);
    /*
    // 等待发送完成或超时
    start_tick = rt_tick_get();
    while (Radio.GetStatus() == RF_TX_RUNNING)
    {
        if (rt_tick_get() - start_tick > rt_tick_from_millisecond(timeout_ms))
        {
            ret = RT_ETIMEOUT;
            Radio.Standby();  // 中止发送
            break;
        }
        rt_thread_mdelay(1);
    }

    Radio.Rx(0);
    */
    // 解锁
    ret = rt_mutex_release(&lora_mutex);
    if (ret != RT_EOK){
        rt_kprintf("lora_driver_send: Failed to release lora_mutex, ret = %d\n", ret);
        return ret;
    }
    
    return ret;
}

rt_err_t lora_driver_set_mod_params(const lora_mod_params_t *params)
{
    if (params == RT_NULL)
        return RT_ERROR;
    
    // 验证参数范围
    if (params->spreading_factor < 7 || params->spreading_factor > 12 ||
        params->bandwidth > 2 || params->coding_rate < 1 || params->coding_rate > 4)
    {
        rt_kprintf("lora_driver_set_mod_params: Invalid params%d\n");
        return RT_ERROR;
    }
    
    // 加锁保证线程安全
    rt_err_t ret = rt_mutex_take(&lora_mutex, RT_WAITING_FOREVER);
    if (ret != RT_EOK){
        rt_kprintf("lora_driver_set_mod_params: Failed to take lora_mutex, ret = %d\n", ret);
        return ret;
    }

    // 保存参数
    memcpy(&lora_mod_params, params, sizeof(lora_mod_params_t));
    
    // 进入待机模式以应用新参数
    Radio.Standby();
    
    // 配置频率
    Radio.SetChannel(params->frequency);
    
    // 配置调制参数
    Radio.SetTxConfig(MODEM_LORA, params->tx_power, 
                    0,          //fdev
                    params->bandwidth, params->spreading_factor, params->coding_rate, params->preamble_len, 
                    0,          //fixlen
                    params->crc_en, 
                    0,          //freqHopOn
                    0,          //hopPeriod
                    params->invert_iq, params->timeout);	                     
    
    Radio.SetRxConfig(MODEM_LORA, params->bandwidth,params->spreading_factor,
                    params->coding_rate, 0, params->preamble_len,
                    0,          //symbTimeout,
                    false,      //fixLen,
                    0,          //payloadLen,
                    params->crc_en,
                    0,          //freqHopOn,
                    0,          //hopPeriod,
                    params->invert_iq,
                    false       //rxContinuous
            );	

    // 重新进入接收模式
    Radio.Rx(0);
    
    // 解锁
    ret = rt_mutex_release(&lora_mutex);
    if (ret != RT_EOK){
        rt_kprintf("lora_driver_set_mod_params: Failed to release lora_mutex, ret = %d\n", ret);
        return ret;
    }
    
    return RT_EOK;
}

rt_err_t lora_driver_sleep(void)
{
    rt_mutex_take(&lora_mutex, RT_WAITING_FOREVER);
    
    // 进入睡眠模式
    Radio.Sleep();
    
    rt_mutex_release(&lora_mutex);
    
    return RT_EOK;
}

rt_err_t lora_driver_wakeup(void)
{
    rt_mutex_take(&lora_mutex, RT_WAITING_FOREVER);
    
    // 唤醒设备
    Radio.Standby();
    
    // 开启接收模式
    Radio.Rx(0);
    
    rt_mutex_release(&lora_mutex);
    
    return RT_EOK;
}
    
rt_err_t lora_driver_release_mutex(void)
{
    return rt_mutex_release(&lora_mutex);
}
