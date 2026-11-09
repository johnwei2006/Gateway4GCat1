#ifndef __MODBUS_MANAGER_H
#define __MODBUS_MANAGER_H
#include <rtthread.h>
#include "modbus_parser.h"

#define MODBUS_RESP_TIMEOUT_MS    1000
#define MODBUS_MAX_RETRY          3
#define MODBUS_TURNAROUND_DELAY_MS 50
#define MODBUS_ERR_EXCEPTION      -10

int modbus_manager_init(void);
/*normal modbus request, both for read and write*/
rt_err_t modbus_request(uint8_t *data, uint16_t len, modbus_request_message_t *modbus_request_message, modbus_response_message_t *modbus_message);
/*modbus request setting for devices with setting type*/
rt_err_t modbus_request_setting(uint8_t *data, uint16_t len, uint8_t *setting_value, modbus_request_message_t *modbus_request_message, modbus_response_message_t *modbus_response_message);
rt_err_t rs485_response_callback(uint8_t *data, uint16_t len);
#endif