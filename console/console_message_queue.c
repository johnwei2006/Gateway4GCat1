#include "console_message_queue.h"

/**
 * @brief  Message queue handle (console ? other modules)
 */
rt_mq_t from_console_mq = RT_NULL;

/**
 * @brief  Message queue handle (other modules ? console)
 */
rt_mq_t to_console_mq = RT_NULL;

/**
 * @brief  Mutex handle for protecting from_console_mq concurrent access
 */
rt_mutex_t from_console_mutex = RT_NULL;

/**
 * @brief  Mutex handle for protecting to_console_mq concurrent access
 */
rt_mutex_t to_console_mutex = RT_NULL;

/**
 * @brief  Put a console data message into from_console_mq (console ? other modules)
 * @param  console_data: Pointer to console_data_t structure (contains received UART data)
 * @retval RT_EOK: Success, other values: Failed (refer to RT-Thread error codes)
 */
int put_from_console_message(console_data_t* console_data)
{
    // Check for invalid null pointers (prevent illegal memory access)
    if (console_data == RT_NULL || from_console_mq == RT_NULL)
    {
        rt_kprintf("put_from_console_message: invalid parameter (NULL pointer)\n");
        return -RT_ERROR;
    }
    
    // Send message to from_console_mq, message length = size of console_data_t structure
    rt_err_t result = rt_mq_send(from_console_mq, console_data, sizeof(console_data_t));
    if (result != RT_EOK)
    {
        rt_kprintf("put_from_console_message failed! err: %d\n", result);
    }
    return result;
}

/**
 * @brief  Get a console data message from from_console_mq (console ? other modules)
 * @param  console_data: Pointer to console_data_t structure for storing received message
 * @retval RT_EOK: Success, other values: Failed (refer to RT-Thread error codes)
 */
rt_err_t get_from_console_message(console_data_t* console_data)
{
    // Check for invalid null pointers (prevent illegal memory access)
    if (console_data == RT_NULL || from_console_mq == RT_NULL)
    {
        rt_kprintf("get_from_console_message: invalid parameter (NULL pointer)\n");
        return -RT_ERROR;
    }
    
    // Receive message from from_console_mq (no waiting, return immediately if no message)
    rt_err_t result = rt_mq_recv(from_console_mq, console_data, sizeof(console_data_t), RT_WAITING_NO);
    if (result != RT_EOK)
    {
        //rt_kprintf("get_from_console_message failed! err: %d\n", result);
    }
    return result;
}

/**
 * @brief  Put a console data message into to_console_mq (other modules ? console)
 * @param  console_data: Pointer to console_data_t structure (contains data to send via UART)
 * @retval RT_EOK: Success, other values: Failed (refer to RT-Thread error codes)
 */
int put_to_console_message(console_data_t* console_data)
{
    // Check for invalid null pointers (prevent illegal memory access)
    if (console_data == RT_NULL || to_console_mq == RT_NULL)
    {
        rt_kprintf("put_to_console_message: invalid parameter (NULL pointer)\n");
        return -RT_ERROR;
    }
    
    // Send message to to_console_mq, message length = size of console_data_t structure
    rt_err_t result = rt_mq_send(to_console_mq, console_data, sizeof(console_data_t));
    if (result != RT_EOK)
    {
        rt_kprintf("put_to_console_message ERR %d\n", result);
    }
    return result;
}

/**
 * @brief  Get a console data message from to_console_mq (other modules ? console)
 * @param  console_data: Pointer to console_data_t structure for storing received message
 * @retval RT_EOK: Success, other values: Failed (refer to RT-Thread error codes)
 */
rt_err_t get_to_console_message(console_data_t* console_data)
{
    // Check for invalid null pointers (prevent illegal memory access)
    if (console_data == RT_NULL || to_console_mq == RT_NULL)
    {
        rt_kprintf("get_to_console_message: invalid parameter (NULL pointer)\n");
        return -RT_ERROR;
    }
    
    // Receive message from to_console_mq (no waiting, return immediately if no message)
    rt_err_t result = rt_mq_recv(to_console_mq, console_data, sizeof(console_data_t), RT_WAITING_NO);
    if (result != RT_EOK)
    {
        //rt_kprintf("get_to_console_message failed! err: %d\n", result);
    }
    return result;
}

/**
 * @brief  Initialize console message queues and related mutexes
 * @note   This function must be called before using any console message queue operations
 * @retval None
 */
void console_message_queue_init()
{
    // Create from_console_mq: 
    // Name: "from_console_mq", Single message size: sizeof(console_data_t), 
    // Queue length: MQ_CONSOLE_MSG_SIZE, Flag: FIFO (first in first out)
    from_console_mq = rt_mq_create("from_console_mq",  
                                   sizeof(console_data_t), 
                                   MQ_CONSOLE_MSG_SIZE, 
                                   RT_IPC_FLAG_FIFO);
    if (from_console_mq == RT_NULL)
    {
        rt_kprintf("initialize from_console_mq failed\n");
    }
    
    // Create to_console_mq:
    // Name: "to_console_mq", Single message size: sizeof(console_data_t),
    // Queue length: MQ_CONSOLE_MSG_SIZE, Flag: FIFO (first in first out)
    to_console_mq = rt_mq_create("to_console_mq", 
                                 sizeof(console_data_t), 
                                 MQ_CONSOLE_MSG_SIZE, 
                                 RT_IPC_FLAG_FIFO);
    if (to_console_mq == RT_NULL)
    {
        rt_kprintf("initialize to_console_mq failed\n");
    }

    // Create from_console_mutex for protecting concurrent access to from_console_mq
    from_console_mutex = rt_mutex_create("from_console_mtx", RT_IPC_FLAG_FIFO);
    if (from_console_mutex == RT_NULL)
    {
        rt_kprintf("initialize from_console_mutex failed\n");
    }
    
    // Create to_console_mutex for protecting concurrent access to to_console_mq
    to_console_mutex = rt_mutex_create("to_console_mtx", RT_IPC_FLAG_FIFO);
    if (to_console_mutex == RT_NULL)
    {
        rt_kprintf("initialize to_console_mutex failed\n");
    }
}