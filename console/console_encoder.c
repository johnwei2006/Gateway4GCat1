#include "console_encoder.h"
#include <rtthread.h>
#include "../utils/utils.h"
#include <string.h>
#include <stdint.h>

extern console_command_t console_command_standard[];
extern int console_command_standard_size;
/* console buffer to console command
 *  format :  #RD KEY
 *            #ST KEY VALUE
 */
rt_bool_t console_decode(console_data_t*    console_data,    console_command_t* console_command)
{
    //1. separate command buffer
    char *command = strtok(console_data->buffer, COMMAND_VALUE_SPLIT);
    char *command_key = strtok(NULL, COMMAND_VALUE_SPLIT);
    char *command_value = NULL;
    if (command != NULL && command_key != NULL) {
        // 公式：command_key 地址 + command_key 长度 + 1（跳过被 strtok 替换的 \0 分割符）
        command_value = command_key + strlen(command_key) + 1;
    }
    if(command == RT_NULL || command_key == RT_NULL){
        rt_kprintf("command format error: %s\r\n", console_data->buffer);
        return RT_FALSE;
    }
    
    //2. check prefix : #RD/#ST
    if(strncasecmp(command, COMMAND_PREFIX_READ, rt_strlen(COMMAND_PREFIX_READ)) == 0){
        console_command->action = COMMAND_READ;
    } else if(strncasecmp(command, COMMAND_PREFIX_WRITE, rt_strlen(COMMAND_PREFIX_WRITE)) == 0){
        console_command->action = COMMAND_WRITE;
    } else if(strncasecmp(command, COMMAND_PREFIX_MODBUS, rt_strlen(COMMAND_PREFIX_MODBUS)) == 0){
        console_command->action = COMMAND_MODBUS;
    } else if(strncasecmp(command, COMMAND_PREFIX_SENSOR_READ, rt_strlen(COMMAND_PREFIX_SENSOR_READ)) == 0){
        console_command->action = COMMAND_SENSOR_READ;
    } else {
        rt_kprintf("command prefix error: %s\r\n", console_data->buffer);
        return RT_FALSE;
    }

    //3. get command and value
    strcpy(console_command->key, command_key);
    console_command->prefix_key_length = rt_strlen(COMMAND_PREFIX_READ) + rt_strlen(COMMAND_VALUE_SPLIT) + rt_strlen(command_key);

    if(console_command->action == COMMAND_WRITE){
        if(command_value == RT_NULL){
            rt_kprintf("command value missing: %s\r\n", console_data->buffer);
            return RT_FALSE;
        }
        strcpy(console_command->value, command_value);
    } else if(console_command->action == COMMAND_MODBUS){
        if(command_value == RT_NULL){
            rt_kprintf("command value missing: %s\r\n", console_data->buffer);
            return RT_FALSE;
        }
        strcpy(console_command->value, command_value);
    } else if(console_command->action == COMMAND_SENSOR_READ){
        //do nothing, sensor read command may not have value
    }   
    //3. decode key and value
    //char *key_group = strtok(key, COMMAND_KEY_SPLIT);
	return RT_TRUE;
};

// console command to console buffer
rt_bool_t console_encode(console_command_t *console_command, console_data_t *console_data){
    //1. encode command prefix: #RD/#ST
    if(console_command->action == COMMAND_READ){
        strcpy(console_data->buffer, COMMAND_PREFIX_READ);
    } else if(console_command->action == COMMAND_WRITE){
        strcpy(console_data->buffer, COMMAND_PREFIX_WRITE);
    } else if(console_command->action == COMMAND_MODBUS){
        strcpy(console_data->buffer, COMMAND_PREFIX_MODBUS);
    } else if(console_command->action == COMMAND_SENSOR_READ){
        strcpy(console_data->buffer, COMMAND_PREFIX_SENSOR_READ);
    } else {
        rt_kprintf("command action error: %d\r\n", console_command->action);
        return RT_FALSE;
    }

    //2. COMMAND_VALUE_SPLIT
    strcat(console_data->buffer, COMMAND_VALUE_SPLIT);

    //3. encode key
    strcat(console_data->buffer, console_command->key);
    //4. encode value if command is #ST or #MD
    if(console_command->value != RT_NULL){
        strcat(console_data->buffer, COMMAND_VALUE_SPLIT);
        strcat(console_data->buffer, console_command->value);
    }
    console_data->size = rt_strlen(console_data->buffer);
    return RT_TRUE;
};