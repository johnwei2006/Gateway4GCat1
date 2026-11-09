#pragma once 
#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include <stdint.h>
#include <rtdef.h>
#include <rtthread.h>

#define COMMAND_BUFFER_SIZE          1024
#define COMMAND_TIMEROUT             RT_TICK_PER_SECOND / 2
#define CONSOLE_UART_NAME            "uart1"
#define COMMAND_PREFIX_READ          "#RD"
#define COMMAND_PREFIX_WRITE         "#ST"
#define COMMAND_PREFIX_MODBUS        "#MD"
#define COMMAND_PREFIX_SENSOR_READ   "#SR"
#define COMMAND_VALUE_SPLIT          " "
#define COMMAND_KEY_SPLIT            "."

typedef enum{
  COMMAND_READ   = 0x01,
  COMMAND_WRITE  = 0x02,
  COMMAND_MODBUS = 0x03,
  COMMAND_SENSOR_READ = 0x04
} command_action_t;

typedef struct
{
    uint8_t             id;         //id for specific command
    command_action_t    action;
    char                *key;
    char                *value;
    uint16_t            prefix_key_length; 
} console_command_t;
/*
typedef struct
{
    command_action_t    action;
    const char          *command_name;
    void                (*execute)(command_action_t action, uint8_t* args);
} command_t;
*/
//命令标志
typedef enum{
  MENU_CMD_READ  = 0x01,
  MENU_CMD_WRITE = 0x02,
} menu_cmd_flag_t;
//菜单命令

typedef struct _console_data{
    uint16_t size;
    uint8_t  buffer[COMMAND_BUFFER_SIZE];
} console_data_t;

void console_init();
void from_console_thread_entry(void *parameter);
void to_console_thread_entry(void *parameter);
rt_err_t uart_input(rt_device_t dev, rt_size_t size);
#endif //__CONSOLE_H
