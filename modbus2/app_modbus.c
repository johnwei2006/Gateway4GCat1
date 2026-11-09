
#include "app_modbus.h"
#include <rtdevice.h>

#define COLLECT_MODBUS_SENSOR_MAXCOUNT (2)  
#define POWER_ON 1
#define POWER_OFF 0
#define LOWPOWER_485            (0x01 << 1)

float modbus_collect_data(uint8_t channel, uint8_t need_power_on_ms, struct rs485_arg *rs485_arg)
{
    bool modbus_result;
    float collect_buf;
    
    board_485_lowpower();

    rt_mutex_take(modbus_mutex, RT_WAITING_FOREVER);
    //board_power_ctrl(channel, POWER_ON);
    rt_thread_mdelay(need_power_on_ms);

    /* Resend numbers*/
    for (uint8_t count_retry = 0; count_retry < COLLECT_MODBUS_SENSOR_MAXCOUNT; count_retry++)
    {
        modbus_result = board_485_collect(&collect_buf, rs485_arg, 500);
        if (true == modbus_result)
        {
            break;
        } else {
            rt_kprintf("Retrying...\r\n");
        }
    }

    //board_power_ctrl(channel, POWER_OFF);
    rt_mutex_release(modbus_mutex);
    return collect_buf;
}

void app_modbus_init(void *context) {
    modbus_mutex = rt_mutex_create("modbus_mutex", RT_IPC_FLAG_FIFO);
}