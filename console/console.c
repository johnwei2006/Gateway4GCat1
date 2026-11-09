#include "console.h"
#include <rtdevice.h>
#include "console_message_queue.h"
#include <string.h>

// Global variables - keep original
static rt_thread_t              from_console_thread = RT_NULL;
static rt_thread_t              to_console_thread = RT_NULL;
static rt_device_t              console_device = RT_NULL;
static struct rt_semaphore      rx_sem_console;
static rt_size_t                uart_input_size = 0;
static rt_bool_t                console_thread_running = RT_TRUE;

/**
 * @brief  UART receive interrupt callback (fast in fast out)
 * @param  dev: UART device handle
 * @param  size: Received data length
 * @retval RT_EOK on success
 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    if (dev == RT_NULL || size == 0 || dev != console_device)
        return -RT_ERROR;
		
    uart_input_size = size;   
    rt_sem_release(&rx_sem_console);
    return RT_EOK;
}


/**
 * @brief Console UART receive thread (DMA mode)
 * @param parameter Thread parameters (unused)
 * @return None
 */
void from_console_thread_entry(void *parameter)
{
    console_data_t g_console_data;     /* Local data structure for UART reception */
    rt_err_t ret = RT_EOK;             /* Function return status */
    rt_base_t level;                   /* Interrupt level for critical section */

    /* Debug assert: Ensure UART device is valid */
    RT_ASSERT(console_device != RT_NULL);
    rt_kprintf("[CONSOLE] from_console_thread start, using %s (DMA Receive)\n", CONSOLE_UART_NAME);

    /* Main thread loop */
    while (console_thread_running)
    {
        /* Wait for DMA receive signal from interrupt */
        rt_sem_take(&rx_sem_console, RT_WAITING_FOREVER);

        /* Critical section: Read and reset data length atomically */
        level = rt_hw_interrupt_disable();
        rt_size_t read_size = uart_input_size;
        uart_input_size = 0;
        rt_hw_interrupt_enable(level);

        /* Prevent buffer overflow */
        if (read_size > COMMAND_BUFFER_SIZE - 1)
            read_size = COMMAND_BUFFER_SIZE - 1;

        /* Clear data structure to avoid dirty data */
        rt_memset(&g_console_data, 0, sizeof(console_data_t));

        /* Read real data from UART driver */
        rt_size_t actual_read = rt_device_read(console_device, -1, g_console_data.buffer, read_size);
        if (actual_read > 0)
        {
            /* Set data length and add string terminator */
            level = rt_hw_interrupt_disable();
            g_console_data.size = actual_read;
            g_console_data.buffer[g_console_data.size] = '\0';
            rt_hw_interrupt_enable(level);

            /* Debug: Print received hex data *
            for (rt_size_t i = 0; i < g_console_data.size; i++)
            {
                rt_kprintf("0x%02X ", g_console_data.buffer[i]);
            }*/

            /* Process \r \n line break characters */
            if (g_console_data.size > 0)
            {
                rt_size_t end_idx = g_console_data.size - 1;
                if (g_console_data.buffer[end_idx] == '\n' || g_console_data.buffer[end_idx] == '\r')
                {
                    /* Remove last line break character */
                    g_console_data.size--;

                    /* Remove the second line break if exists (for \r\n) */
                    if (g_console_data.size > 0 &&
                        (g_console_data.buffer[g_console_data.size - 1] == '\n' ||
                         g_console_data.buffer[g_console_data.size - 1] == '\r'))
                    {
                        g_console_data.size--;
                    }

									// Force reserve at least 1 byte (ensure the # character is not lost)
                    if (g_console_data.size == 0) g_console_data.size = 1;
                    g_console_data.buffer[g_console_data.size] = '\0';
										
                    /* Send valid message to queue */
                    if (g_console_data.size > 0)
                    {
                        ret = put_from_console_message(&g_console_data);
                        if (ret != RT_EOK)
                        {
                            rt_kprintf("[CONSOLE] Put msg failed! err: %d\n", ret);
                        }
                    }
                }
            }
        }
    }

    /* Release resources when thread exits */
    rt_kprintf("[CONSOLE] Thread exit, release resource\n");
    rt_device_close(console_device);
    rt_sem_detach(&rx_sem_console);
}


/**
 * @brief UART console send thread
 * @param parameter unused
 * @return none
 */
void to_console_thread_entry(void *parameter)
{
    rt_kprintf("[TO CONSOLE THREAD] Entry\n");
    console_data_t to_console_data;  /* Buffer for receiving data from output message queue */
    
    while (console_thread_running)
    {
        rt_thread_delay(100);
        
        /* Get data from message queue */
        if (get_to_console_message(&to_console_data) == RT_EOK)
        {
            rt_kprintf("[CONSOLE] Write to UART, size: %d\n", to_console_data.size);
            rt_size_t actual_write = rt_device_write(console_device, 0, to_console_data.buffer, to_console_data.size);
            
            /* Check if write completed successfully */
            if (actual_write != to_console_data.size)
            {
                rt_kprintf("[CONSOLE] Write failed! actual: %d, expect: %d\n", actual_write, to_console_data.size);
            }
        }
    }
}
/**
 * @brief  Console initialization (UART DMA + Threads + IPC)
 * @param  None
 * @retval None
 */
void console_init(void)
{
    rt_err_t ret = RT_EOK;

    /* Initialize message queues for console TX/RX */
    console_message_queue_init();
    if (from_console_mq == RT_NULL || to_console_mq == RT_NULL)
    {
        rt_kprintf("[CONSOLE] Message queue init failed! Exit init!\n");
        return;
    }
    rt_kprintf("[CONSOLE] Message queue init success!\n");

    /* Find UART device */
    console_device = rt_device_find(CONSOLE_UART_NAME);
    if (!console_device)
    {
        rt_kprintf("[CONSOLE] Find %s failed!\n", CONSOLE_UART_NAME);
        return;
    }

    /* Initialize RX semaphore (used for DMA RX signal) */
    ret = rt_sem_init(&rx_sem_console, "rx_sem_console", 0, RT_IPC_FLAG_FIFO);
    if (ret != RT_EOK)
    {
        rt_kprintf("[CONSOLE] Init sem failed! err: %d\n", ret);
        return;
    }

    /* Open UART with DMA RX mode */
    ret = rt_device_open(console_device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_STREAM);
    if (ret != RT_EOK)
    {
        rt_kprintf("[CONSOLE] Open %s failed! err: %d\n", CONSOLE_UART_NAME);
        rt_sem_detach(&rx_sem_console);
        return;
    }

    /* UART configuration: 115200 8N1 */
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = BAUD_RATE_115200;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
    config.parity = PARITY_NONE;
    ret = rt_device_control(console_device, RT_DEVICE_CTRL_CONFIG, &config);
    if (ret != RT_EOK)
    {
        rt_kprintf("[CONSOLE] Config %s failed! err: %d\n", CONSOLE_UART_NAME, ret);
        rt_device_close(console_device);
        rt_sem_detach(&rx_sem_console);
        return;
    }

    /* Set UART RX callback (DMA ready handler) */
    ret = rt_device_set_rx_indicate(console_device, uart_input);
    if (ret != RT_EOK)
    {
        rt_kprintf("[CONSOLE] Set rx indicate failed! err: %d\n", ret);
        rt_device_close(console_device);
        rt_sem_detach(&rx_sem_console);
        return;
    }

    /* Create and start UART RX thread */
    from_console_thread = rt_thread_create("from_console_thread",
                                      from_console_thread_entry,
                                      RT_NULL,
                                      2048,
                                      25,
                                      10);
    if (from_console_thread != RT_NULL)
    {
        rt_thread_startup(from_console_thread);
        rt_kprintf("[FROM CONSOLE THREAD] %s init success\n", CONSOLE_UART_NAME);
    }

    /* Create and start UART TX thread */
    to_console_thread = rt_thread_create("to_console_thread",
                                      to_console_thread_entry,
                                      RT_NULL,
                                      2048,
                                      25,
                                      10);
    if (to_console_thread != RT_NULL)
    {
        rt_thread_startup(to_console_thread);
        rt_kprintf("[TO CONSOLE THREAD] %s init success\n", CONSOLE_UART_NAME);
    }
    else
    {
        rt_kprintf("[CONSOLE] Create thread failed!\n");
        rt_device_close(console_device);
        rt_sem_detach(&rx_sem_console);
    }
}

INIT_APP_EXPORT(console_init);