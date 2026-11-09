#ifndef __APP_MODBUS_H	
#define __APP_MODBUS_H

#include "board_rs485.h"
#include "rtthread.h"

static rt_mutex_t              modbus_mutex;   

void app_modbus_init(void *context) ;
float modbus_collect_data(uint8_t channel, uint8_t need_power_on_ms, struct rs485_arg *rs485_arg);

#endif//!@__APP_MODBUS_H
