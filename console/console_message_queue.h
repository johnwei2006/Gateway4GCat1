#ifndef __CONSOLE_MESSAGE_QUEUE_H__
#define __CONSOLE_MESSAGE_QUEUE_H__

#include <rtdef.h>
#include "rthw.h"
#include "console.h"

/**
 * @brief  External message queue handles (from console to other modules)
 */
extern rt_mq_t from_console_mq;

/**
 * @brief  External message queue handles (from other modules to console)
 */
extern rt_mq_t to_console_mq;

/**
 * @brief  External mutex handle for protecting from_console_mq access
 */
extern rt_mutex_t from_console_mutex;

/**
 * @brief  External mutex handle for protecting to_console_mq access
 */
extern rt_mutex_t to_console_mutex;

/**
 * @brief  Maximum number of messages in console message queue
 *         (each message is console_data_t type)
 */
#define MQ_CONSOLE_MSG_SIZE 10

/**
 * @brief  Maximum length of single console message (reserved for future use)
 */
#define MQ_CONSOLE_MSG_MAX_LENGTH  200

/**
 * @brief  Initialize console message queues and related mutexes
 * @note   This function must be called before using any console message queue operations
 * @retval None
 */
void console_message_queue_init(void);

/**
 * @brief  Put a console data message into from_console_mq (console ? other modules)
 * @param  console_data: Pointer to console_data_t structure (contains received UART data)
 * @retval RT_EOK: Success, other values: Failed (refer to RT-Thread error codes)
 */
int put_from_console_message(console_data_t* console_data);

/**
 * @brief  Put a console data message into to_console_mq (other modules ? console)
 * @param  console_data: Pointer to console_data_t structure (contains data to send via UART)
 * @retval RT_EOK: Success, other values: Failed (refer to RT-Thread error codes)
 */
int put_to_console_message(console_data_t* console_data);

/**
 * @brief  Get a console data message from from_console_mq (console ? other modules)
 * @param  console_data: Pointer to console_data_t structure for storing received message
 * @retval RT_EOK: Success, other values: Failed (refer to RT-Thread error codes)
 */
rt_err_t get_from_console_message(console_data_t* console_data);

/**
 * @brief  Get a console data message from to_console_mq (other modules ? console)
 * @param  console_data: Pointer to console_data_t structure for storing received message
 * @retval RT_EOK: Success, other values: Failed (refer to RT-Thread error codes)
 */
rt_err_t get_to_console_message(console_data_t* console_data);

#endif /*CONSOLE_MESSAGE_QUEUE_H_*/