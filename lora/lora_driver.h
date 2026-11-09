#ifndef __LORA_DRIVER_H__
#define __LORA_DRIVER_H__

#include <rtthread.h>
#include <lora-radio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  RF_BW_125KHZ = 0,
  RF_BW_250KHZ = 1,
  RF_BW_500KHZ = 2,
} rf_bw_t;         
	
typedef enum
{
  RF_DR_SF5 = 5,
  RF_DR_SF6 = 6,
  RF_DR_SF7 = 7,
  RF_DR_SF8 = 8,
  RF_DR_SF9 = 9,
  RF_DR_SF10 = 10,
  RF_DR_SF11 = 11,
  RF_DR_SF12 = 12,
} rf_datr_t;
/**
 * LoRa调制参数结构体
 */
typedef struct
{
    uint32_t    frequency;          // 频率 (Hz)
    int8_t      tx_power;           // 发射功率 (dBm)
    uint8_t     spreading_factor;   // 扩频因子 (SF5 to SF12)
    uint8_t     bandwidth;          // 带宽 (0=125kHz, 1=250kHz, 2=500kHz)
    uint8_t     coding_rate;        // 编码率 (1=4/5, 2=4/6, 3=4/7, 4=4/8)
    uint16_t    preamble_len;       // 前导码长度
    bool        crc_en;             // CRC使能
    bool        invert_iq;          // IQ反转
    uint32_t    timeout;  
} lora_mod_params_t;

// 回调函数声明
static void on_rx_done(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
static void on_rx_timeout(void);
static void on_rx_error(void);

// 定义函数指针类型（回调函数签名）
typedef void (*on_cad_done_callback)(bool);
typedef void (*on_tx_done_callback)();
typedef void (*on_tx_timeout_callback)();
void lora_power(bool enable);
/**
 * @brief 初始化LoRa驱动
 * @param config 配置参数
 * @return 成功返回RT_EOK，失败返回错误码
 */
rt_err_t lora_driver_init(const lora_mod_params_t *mod_params, on_cad_done_callback cad_done_callback, on_tx_done_callback tx_done_callback, on_tx_timeout_callback tx_timeout_callback);

/**
 * @brief 设备PIN重置
 * @return 成功返回RT_EOK
 */
rt_err_t lora_power_pin_init(void);
/**
 * @brief 反初始化LoRa驱动
 * @return 成功返回RT_EOK
 */
rt_err_t lora_driver_deinit(void);

/**
 * @brief 执行CAD检测
 * @param timeout_ms 超时时间(ms)
 * @return 检测到空闲返回RT_EOK，忙返回RT_BUSY，超时返回RT_ETIMEOUT
 */
rt_err_t lora_driver_cad(uint32_t timeout_ms);

/**
 * @brief 发送原始数据
 * @param data 要发送的数据
 * @param len 数据长度
 * @param timeout_ms 超时时间(ms)
 * @return 成功返回RT_EOK，失败返回错误码
 */
rt_err_t lora_driver_send(uint8_t *data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief 接收原始数据
 * @param rx_data 接收数据结构体
 * @param timeout_ms 超时时间(ms)
 * @return 成功返回RT_EOK，失败返回错误码
 */
//rt_err_t lora_driver_receive(LoRaRxData_t *rx_data, uint32_t timeout_ms);

/**
 * @brief 设置调制参数
 * @param params 调制参数
 * @return 成功返回RT_EOK
 */
rt_err_t lora_driver_set_mod_params(const lora_mod_params_t *params);

/**
 * @brief 进入睡眠模式
 * @return 成功返回RT_EOK
 */
rt_err_t lora_driver_sleep(void);

/**
 * @brief 唤醒设备
 * @return 成功返回RT_EOK
 */
rt_err_t lora_driver_wakeup(void);

/**
 * @brief 释放互斥锁
 * @return 成功返回RT_EOK
 */
rt_err_t lora_driver_release_mutex(void);

#ifdef __cplusplus
}
#endif

#endif /* __LORA_DRIVER_H__ */