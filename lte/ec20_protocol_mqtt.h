/*
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe: EC20 MQTT protocol header
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
 * 2024-05-23     user           first version
 */
#ifndef __EC20_PROTOCOL_MQTT_H
#define __EC20_PROTOCOL_MQTT_H

#include "lte_def.h"
#include <rtthread.h>
#include "ec20_hal.h"

// MQTT接收回调函数类型
typedef void (*mqtt_recv_callback)(const char *data, uint16_t len);

// MQTT相关函数声明
void ec20_set_mqtt_params(const char *server_ip, uint16_t port, const char *client_id, const char *username, const char *password);
void ec20_set_mqtt_commands_first(at_response_t resp);
at_command_process_t **ec20_get_mqtt_connect_commands(void);
at_command_process_t **ec20_get_mqtt_send_data_commands(void);
at_command_process_t **ec20_get_mqtt_read_data_commands(void);
int ec20_prepare_mqtt_publish_data(const char *topic, const char *data, uint16_t len, uint8_t qos);
int ec20_prepare_mqtt_subscribe_data(const char *topic, uint8_t qos);
int ec20_prepare_mqtt_unsubscribe_data(const char *topic);
void ec20_set_mqtt_recv_callback(mqtt_recv_callback function);

#endif