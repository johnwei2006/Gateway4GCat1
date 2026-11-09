#include "console_manager.h"
#include "console_encoder.h"
#include <rtthread.h>
#include <string.h>
#include <ctype.h>
#include "../utils/utils.h"
#include "../config/config_manager.h"
#include "console_message_queue.h"
#include "../modbus/modbus_manager.h"
#include "../service_udp/service_gateway_sensor_report.h"

/* Console manager thread handle, global for thread management */
static rt_thread_t console_manager_thread = RT_NULL;

/**
 * @brief  Initialize console manager module
 * @note   Create and start console manager thread, print usage guide
 * @retval None
 */
void console_manager_init()
{
    // Create console manager thread: name, entry, param, stack(4096), priority(10), tick(10)
    console_manager_thread = rt_thread_create("console_manager_thread", 
                                              console_manager_thread_entry, 
                                              RT_NULL, 
                                              4096, 
                                              10, 
                                              10);
    if (console_manager_thread != RT_NULL)
    {
       rt_thread_startup(console_manager_thread); // Start the thread
       rt_kprintf("console_manager_thread start success!\n");
    } 
    else 
    {
       rt_kprintf("create console_manager_thread Entry failed!\n");
    }
    print_usage(); // Print console command usage guide
};


static void safe_free(void **ptr)
{
    if (ptr != NULL && *ptr != RT_NULL)
    {
        rt_free(*ptr);
        *ptr = RT_NULL;
    }
}

/**
 * @brief  Console manager thread entry function
 * @note   Core logic: receive console data from MQ -> decode command -> execute R/W config -> encode response
 * @param  parameter: Thread parameter (not used here, RT_NULL)
 * @retval None
 */
static void console_manager_thread_entry(void *parameter)
{
    // Define stack console_data struct (auto memory manage, no malloc/free needed)
    console_data_t console_data;
    while (1)
    {
        // Step 1: Receive data from from_console_mq (no waiting, RT_WAITING_NO)
        rt_err_t mq_result = get_from_console_message(&console_data);
        if(mq_result == RT_EOK)
        {
            console_command_t   *console_command = RT_NULL;
            char                *cmd_key = RT_NULL;
            char                *cmd_value = RT_NULL;
            char                *config_value = RT_NULL;

            console_command = rt_malloc(sizeof(console_command_t));
            if (console_command == RT_NULL)
            {
                rt_kprintf("console_command malloc failed!\n");
                goto clean_exit;
            }

            cmd_key = rt_calloc(COMMAND_KEY_MAX_LENGTH, 1);
            if (cmd_key == RT_NULL)
            {
                rt_kprintf("console_command key calloc failed!\n");
                goto free_command; 
            }


            cmd_value = rt_calloc(COMMAND_VALUE_MAX_LENGTH, 1);
            if (cmd_value == RT_NULL)
            {
                rt_kprintf("console_command value calloc failed!\n");
                goto free_key; 
            }


            console_command->key    = cmd_key;
            console_command->value  = cmd_value;

            // Step 3: Check HELP command, print usage guide
            if(strncasecmp((char*)console_data.buffer, "HELP", rt_strlen("HELP")) == 0)
            {
                print_usage();
                goto clean_free; 
            }

            // Step 4: Decode raw console data to structured command
            rt_bool_t decode_result = console_decode(&console_data, console_command);
            if(!decode_result)
            {
                rt_kprintf("command decode error: %s\r\n", console_data.buffer);
                goto clean_free; 
            }
            switch (console_command->action)
            {
                case COMMAND_READ:
                {

                    for (int i=0; i<COMMAND_KEY_MAX_LENGTH-1 && console_command->key[i]!='\0'; i++)
                    {
                        if (console_command->key[i] < 0x20 || console_command->key[i] > 0x7E)
                        {
                            console_command->key[i] = ' ';
                        }
                    }
                    rt_kprintf("Read command: %s\r\n", console_command->key);


                    config_value = rt_calloc(COMMAND_VALUE_MAX_LENGTH, sizeof(char));
                    if (config_value == RT_NULL)
                    {
                        rt_kprintf("config_value calloc failed!\n");
                        goto clean_free;
                    }


                    uint32_t value_length = COMMAND_VALUE_MAX_LENGTH;
                    rt_bool_t read_result = config_manager_read_config(console_command->key, 
                                                                       config_value, 
                                                                       value_length);

                    if(read_result == RT_TRUE && value_length > 0)
                    {
                        //rt_kprintf("Config value: %s (length: %d)\r\n", config_value, value_length);

                        rt_strncpy(console_command->value, config_value, value_length);
                        console_command->value[value_length-1] = '\0'; 

                        console_encode(console_command, &console_data);
                        put_to_console_message(&console_data);
                    }
                    else
                    {
                        rt_kprintf("Config value not found! key: %s\r\n", console_command->key);
                        rt_memset(&console_data, 0, sizeof(console_data));
                    }
                    safe_free((void**)&config_value); 
                    break;
                }

                case COMMAND_WRITE:
                {
                    rt_kprintf("Write command: %s Value: %s\r\n", console_command->key, console_command->value);
                    if (console_command->key[0] == '\0' || console_command->value[0] == '\0')
                    {
                        rt_kprintf("Write command: key/value is empty!\r\n");
                        strcpy(console_command->value, "FAILED_EMPTY");
                    }
                    else
                    {

                        rt_bool_t write_result = config_manager_write_config(console_command->key, console_command->value);
                        rt_strncpy(console_command->value, write_result ? "SUCCESS" : "FAILED", COMMAND_VALUE_MAX_LENGTH-1);
                        console_command->value[COMMAND_VALUE_MAX_LENGTH-1] = '\0';
                        rt_kprintf("Config write %s!\r\n", write_result ? "success" : "failed");
                    }

           
                    console_encode(console_command, &console_data);
                    put_to_console_message(&console_data);
                    break;
                }

                case COMMAND_MODBUS:
                {
                    rt_kprintf("Modbus command: %s Value: %s\r\n", console_command->key, console_command->value);
                    modbus_request_message_t *modbus_request_message = (modbus_request_message_t*)rt_malloc(sizeof(modbus_request_message_t));
                    if(modbus_request_message == RT_NULL)
                    {
                        rt_kprintf("modbus_request_message malloc failed!\n");
                        goto clean_free;
                    }
                    modbus_response_message_t modbus_response_message = {0};										 
                    rt_err_t modbus_ret = modbus_request((uint8_t*)console_command->value, rt_strlen(console_command->value), modbus_request_message, &modbus_response_message);
                    if(modbus_ret == RT_EOK)
                    {
                        rt_kprintf("console_modbus_message.value: 0x%02X,0x%02X\n", modbus_response_message.value[0],modbus_response_message.value[1]);
                        rt_strncpy(console_command->value, (const char*)modbus_response_message.value, modbus_response_message.byte_number);
                    }
                    else
                    {
                        rt_kprintf("Modbus request failed! err: %d\r\n", modbus_ret);
                        memset(console_command->value, 0, COMMAND_VALUE_MAX_LENGTH);
                    }
                    if(modbus_response_message.byte_number > 0)
                    {
                        console_command->value[modbus_response_message.byte_number-1] = '\0';
                    }

                    rt_free(modbus_request_message);

                    console_encode(console_command, &console_data);
                    put_to_console_message(&console_data);
                    break;
                }

                case COMMAND_SENSOR_READ:
                {
                    rt_kprintf("Sensor read command: %s key(node_id): %u\r\n", console_command->key, console_command->id);
                    service_execute_gateway_sensor_report(NULL, console_command->key);
                    break;
                }

                default:
                    rt_kprintf("Unknown command action: %d\r\n", console_command->action);
                    rt_memset(&console_data, 0, sizeof(console_data));
                    break;
            }

        clean_free:
            safe_free((void**)&cmd_value);
            safe_free((void**)&cmd_key);
            safe_free((void**)&console_command);
            safe_free((void**)&config_value);
        clean_exit:
            rt_memset(&console_data, 0, sizeof(console_data));
            continue;
        free_key:
            safe_free((void**)&cmd_key);
        free_command:
            safe_free((void**)&console_command);
            rt_memset(&console_data, 0, sizeof(console_data));
            continue;
        }
        rt_thread_mdelay(100);
    }
};

/**
 * @brief  Print console command usage guide
 * @note   Show READ/WRITE command format and examples for user operation
 * @retval None
 */
void print_usage()
{
    rt_kprintf("\r\n*************************************\r\n");
    rt_kprintf("Usage: \r\n");
    rt_kprintf("Read config by typing the following: \r\n");
    rt_kprintf("#RD KEY ('KEY' should be separated by '.')\r\n");
    rt_kprintf("Write config by typing the following: \r\n");
    rt_kprintf("#ST KEY VALUE ('KEY' should be separated by '.')\r\n");
    rt_kprintf("Modbus command format: \r\n");
    rt_kprintf("#MD COMMAND_HEX (e.g. 01050000FF008C3A)\r\n");
    rt_kprintf("Sensor read command format: \r\n");
    rt_kprintf("#SR $NODE_ID$ \r\n");

    rt_kprintf("*************************************\r\n");
    // Command examples
    rt_kprintf("Examples: \r\n");
    rt_kprintf("#RD GATEWAY_INFO   \r\n");
    rt_kprintf("#RD GATEWAY_CONFIG\r\n");
    rt_kprintf("#ST GATEWAY_CONFIG $CONFIG_JSON$ \r\n");
    rt_kprintf("#RD CONFIG_NODE\r\n");
    rt_kprintf("#ST CONFIG_NODE $CONFIG_JSON$ \r\n");
    rt_kprintf("#RD CONFIG_SENSOR.$NODE_ID$ \r\n");
    rt_kprintf("#ST CONFIG_SENSOR.$NODE_ID$ $CONFIG_JSON$ \r\n");
    rt_kprintf("#RD CONFIG_DEVICE.$NODE_ID$ \r\n");
    rt_kprintf("#ST CONFIG_DEVICE.$NODE_ID$ $CONFIG_JSON$ \r\n");
    rt_kprintf("#MD COMMAND_HEX 010380400002EC1F \r\n");
    rt_kprintf("#SR $NODE_ID$ \r\n");
     rt_kprintf("*************************************\r\n");
};

INIT_COMPONENT_EXPORT(console_manager_init);