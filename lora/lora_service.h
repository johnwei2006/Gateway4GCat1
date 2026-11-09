#ifndef __LORA_SERVICE_H__
#define __LORA_SERVICE_H__

#include <rtthread.h>
#include "lora_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * LoRa服务配置结构体
 */
typedef struct
{
    uint8_t         device_addr;         // 本机地址
    bool            auto_ack;               // 自动回复确认
    uint32_t        tx_timeout;         // 发送超时(ms)
    uint32_t        rx_timeout;         // 接收超时(ms)
    uint8_t         max_retry;           // 最大重传次数
    lora_mod_params_t mod_params;  // 调制参数
} lora_service_config_t;

typedef enum
{
    LORA_STATE_INIT,        // 初始化状态
    LORA_STATE_CONFIG,      // 配置状态（TX/RX参数）
    LORA_STATE_START_RX,    // 启动接收状态
    LORA_STATE_START_CAD,    // 启动CAD检测状态
    LORA_STATE_CAD_DONE,    // CAD检测完成状态
    LORA_STATE_START_TX,     // 发送状态
    LORA_STATE_TX_DONE,     // 发送完成状态（实际为"准备发送"）
    LORA_STATE_IDLE,        // 空闲状态（信道检测+状态切换）
} lora_state_t;

static void lora_service_on_tx_done(void);
static void lora_service_on_cad_done(bool);
static void lora_service_on_tx_timeout(void);

/**
 * @brief 初始化LoRa服务
 * @param config 服务配置
 * @return 成功返回RT_EOK，失败返回错误码
 */
//rt_err_t lora_service_init(const lora_service_config_t *config);
int lora_service_init(void);
/**
 * @brief 反初始化LoRa服务
 * @return 成功返回RT_EOK
 */
rt_err_t lora_service_deinit(void);

/**
 * @brief 获取当前配置
 * @param config 配置缓冲区
 * @return 成功返回RT_EOK
 */
rt_err_t lora_service_get_config(lora_service_config_t *config);

/**
 * @brief 更新配置参数
 * @param config 新的配置
 * @return 成功返回RT_EOK
 */
rt_err_t lora_service_update_config(const lora_service_config_t *config);


#ifdef __cplusplus
}
#endif
#endif /* __LORA_SERVICE_H__ */
