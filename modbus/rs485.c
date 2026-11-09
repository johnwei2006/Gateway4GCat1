/**
  ******************************************************************************
  * @file    rs485_max13487.c
  * @author  RT-Thread Development Team
  * @brief   RS485 driver for STM32F407 with MAX13487 (Auto Transmit/Receive + DMA RX)
  * @note    MAX13487 supports automatic direction control, no GPIO control for DE/RE pin required
  ******************************************************************************
  */
#include "rs485.h"
#include "modbus_manager.h"
/* ==================== Global Static Variables ==================== */
static rt_device_t             rs485_uart = RT_NULL;   /* UART device handle */
static rt_mutex_t              rs485_mutex;             /* Mutex: Protect shared resources (rx_buf, rx_len) */
rt_uint8_t                     rx_buf[RS485_RX_BUF_SIZE]; /* Raw receive buffer for RS485 data */
static rt_uint32_t             rx_len = 0;             /* Valid data length in raw receive buffer */
static struct rt_semaphore     rx_sem_rs485;            /* Semaphore: Notify RX thread when DMA receive completes */
static volatile rt_size_t      uart_input_size = 0;     /* Data size received by DMA (ISR write, thread read) */
static rt_flag_t               rx_buf_overflow = RT_FALSE; /* Flag: RX buffer overflow status */
static rt_bool_t               rs485_thread_running = RT_TRUE; /* Flag: Control RX thread running state */
static rs485_stats_t           rs485_stats = {0};       /* RS485 communication statistics */

/**
  * @brief  UART DMA receive interrupt callback function (fast in fast out)
  * @param  dev: UART device handle from interrupt
  * @param  size: Total bytes received by DMA
  * @retval RT_EOK: Callback executed successfully
  *         -RT_ERROR: Invalid parameter or device mismatch
  * @note   Only update data size and release semaphore, no heavy processing in interrupt context
  */
static rt_err_t rs485_uart_input(rt_device_t dev, rt_size_t size)
{
    if (dev == RT_NULL || size == 0 || dev != rs485_uart)
        return -RT_ERROR;

    uart_input_size = size;

    rt_sem_release(&rx_sem_rs485);
    return RT_EOK;
}

/**
  * @brief  RS485 receive thread entry (DMA mode)
  * @param  parameter: Thread parameter (unused, set to RT_NULL when create)
  * @note   Process DMA received raw binary data (no printable char filter for Modbus)
  *         Critical fix: Removed ASCII filter to preserve Modbus binary frames
  */
static void rs485_rx_thread(void *parameter)
{
    rt_err_t ret = RT_EOK;
    rt_base_t level;
    rt_size_t read_size = 0;
    rt_size_t actual_read = 0;
    rt_uint32_t accumulated = 0;

    RT_ASSERT(rs485_uart != RT_NULL);
    rt_kprintf("[RS485] RX thread start, using %s (DMA RX + Frame Timeout)\n", RS485_UART_NAME);

    while (rs485_thread_running)
    {
        /* Step 1: Wait for initial DMA data (permanent wait) */
        ret = rt_sem_take(&rx_sem_rs485, RT_WAITING_FOREVER);
        if (ret != RT_EOK || !rs485_thread_running)
            continue;

        /* Step 2: Stabilize DMA-to-ringbuffer transfer before reading */
        rt_thread_mdelay(RS485_DMA_STABILIZE_MS);

        /* Step 3: Atomically read and clear uart_input_size */
        level = rt_hw_interrupt_disable();
        read_size = uart_input_size;
        uart_input_size = 0;
        rt_hw_interrupt_enable(level);

        if (read_size == 0) continue;

        if (read_size > RS485_RX_BUF_SIZE - 1)
        {
            read_size = RS485_RX_BUF_SIZE - 1;
            rs485_stats.rx_overflows++;
            rt_kprintf("[RS485 WARN] DMA overflow, capped to %d\n", read_size);
        }

        /* Step 4: Read initial data chunk */
        rt_mutex_take(rs485_mutex, RT_WAITING_FOREVER);
        actual_read = rt_device_read(rs485_uart, -1, rx_buf, read_size);
        rt_mutex_release(rs485_mutex);

        if (actual_read == 0) continue;
        accumulated = actual_read;

        /* Step 5: Frame timeout detection - accumulate data until bus idle (3.5T gap) */
        while (accumulated < RS485_RX_BUF_SIZE - 1)
        {
            ret = rt_sem_take(&rx_sem_rs485,
                              rt_tick_from_millisecond(RS485_FRAME_TIMEOUT_MS));
            if (ret != RT_EOK)
            {
                /* Timeout: bus idle for 3.5T, frame is complete */
                break;
            }

            /* More data arrived before timeout, read and append */
            rt_thread_mdelay(RS485_DMA_STABILIZE_MS);

            level = rt_hw_interrupt_disable();
            read_size = uart_input_size;
            uart_input_size = 0;
            rt_hw_interrupt_enable(level);

            if (read_size == 0) break;

            if (accumulated + read_size > RS485_RX_BUF_SIZE - 1)
            {
                rs485_stats.rx_overflows++;
                rt_kprintf("[RS485 WARN] Frame overflow at %d bytes\n", accumulated);
                break;
            }

            rt_mutex_take(rs485_mutex, RT_WAITING_FOREVER);
            actual_read = rt_device_read(rs485_uart, -1,
                                         rx_buf + accumulated, read_size);
            rt_mutex_release(rs485_mutex);

            if (actual_read == 0) break;
            accumulated += actual_read;
        }

        /* Step 6: Process complete frame */
        if (accumulated > 0)
        {
            rs485_stats.rx_frames++;
            rs485_stats.rx_bytes += accumulated;

            rt_kprintf("[RS485 RX] Len=%d: ", accumulated);
            for (rt_uint32_t i = 0; i < accumulated; i++)
                rt_kprintf("%02X ", rx_buf[i]);
            rt_kprintf("\n");

            level = rt_hw_interrupt_disable();
            rx_len = accumulated;
            rt_hw_interrupt_enable(level);

            rs485_response_callback(rx_buf, accumulated);
        }
    }

    rt_kprintf("[RS485] RX thread exit\n");
}

/**
  * @brief  Read data from RS485 receive buffer (DMA mode compatible)
  * @param  data: Pointer to user output buffer (store read data)
  * @param  len: User expected read length (bytes)
  * @return Actual read length (bytes):
  *         >0: Read successfully, return actual read bytes
  *         0: No valid data in receive buffer
  *         -1: Invalid parameter (data is NULL or len is 0)
  *         -2: RX buffer overflow occurred (data may be lost)
  * @note   This function is exported for other modules to read RS485 data
  */
rt_int32_t rs485_read(rt_uint8_t *data, rt_uint32_t len)
{
    rt_int32_t read_len = 0;
    rt_base_t level; // Interrupt lock level: protect critical section

    /* Parameter validation */
    if (data == RT_NULL || len == 0)
        return -1;

    /* Handle overflow flag: return error and reset flag */
    if (rx_buf_overflow)
    {
        rx_buf_overflow = RT_FALSE;
        return -2;
    }

    rt_mutex_take(rs485_mutex, RT_WAITING_FOREVER);
    if (rx_len > 0)
    {
        // Read the smaller value of expected len and actual len to avoid overflow
        read_len = (len < rx_len) ? len : rx_len;
        rt_memcpy(data, rx_buf, read_len);

        /* Handle packet sticking: move remaining data to buffer head */
        if (rx_len > read_len)
        {
            rt_memmove(rx_buf, rx_buf + read_len, rx_len - read_len);
            
            // Update rx_len in critical section (avoid interrupt modify)
            level = rt_hw_interrupt_disable();
            rx_len -= read_len;
            rt_hw_interrupt_enable(level);
        }
        else
        {
            rx_len = 0; /* Clear buffer length when all data is read */
        }
    }
    rt_mutex_release(rs485_mutex);

    return read_len;
}
RTM_EXPORT(rs485_read); /* Export function for other modules */

/**
  * @brief  Send data via RS485 bus (DMA mode no change for TX)
  * @param  data: Pointer to user input buffer (data to send)
  * @param  len: Length of data to send (bytes)
  * @return Actual sent length (bytes):
  *         >0: Send successfully, return actual sent bytes
  *         -1: Failed (invalid parameter or UART not initialized)
  * @note   MAX13487 automatic direction control, no DE/RE pin operation required
  */
rt_int32_t rs485_send(rt_uint8_t *data, rt_uint32_t len)
{
    rt_int32_t send_len = -1;
    rt_base_t level;
    rt_uint32_t turnaround_ms;

    /* Parameter validation */
    if (rs485_uart == RT_NULL || data == RT_NULL || len == 0)
        return -1;

    /* Adaptive turnaround: TX time + 3.5T gap + extra margin */
    turnaround_ms = (len * 10 * 1000) / RS485_BAUD_RATE + RS485_FRAME_TIMEOUT_MS + RS485_BUS_TURNAROUND_MS;

    /* Lock mutex: Prevent RX thread preempt during transmission */
    rt_mutex_take(rs485_mutex, RT_WAITING_FOREVER);

    /* Flush stale RX data and clear DMA input size to avoid stale trigger */
    rt_uint8_t dummy_buf[64];
    while (rt_device_read(rs485_uart, -1, dummy_buf, sizeof(dummy_buf)) > 0);

    level = rt_hw_interrupt_disable();
    uart_input_size = 0;
    rt_hw_interrupt_enable(level);

    /* Send data (Blocking mode: wait until transmission complete) */
    send_len = rt_device_write(rs485_uart, 0, data, len);

    /* Wait for TX shift register to empty + RS485 transceiver turnaround */
    rt_thread_mdelay(turnaround_ms);

    /* Unlock mutex */
    rt_mutex_release(rs485_mutex);

    if (send_len > 0)
    {
        rs485_stats.tx_frames++;
        rs485_stats.tx_bytes += send_len;

        rt_kprintf("[RS485 TX] Len=%d, Turnaround=%dms: ", send_len, turnaround_ms);
        for (rt_uint32_t i = 0; i < send_len; i++)
            rt_kprintf("%02X ", data[i]);
        rt_kprintf("\n");
    }
    else
    {
        rt_kprintf("[RS485 TX] Transmission failed!\n");
    }

    return send_len;
}
RTM_EXPORT(rs485_send); /* Export function for other modules */

/**
  * @brief  RS485 deinitialization function
  * @param  None
  * @retval None
  * @note   Stop all RS485 related threads and release system resources
  */
void rs485_deinit(void)
{
    rs485_thread_running = RT_FALSE;

    /* Wake up RX thread to exit loop */
    rt_sem_release(&rx_sem_rs485);
    rt_thread_mdelay(100);

    /* Close and release UART device */
    if (rs485_uart != RT_NULL)
    {
        rt_device_close(rs485_uart);
        rs485_uart = RT_NULL;
    }

    /* Detach IPC resources */
    rt_sem_detach(&rx_sem_rs485);
    if (rs485_mutex != RT_NULL)
    {
        rt_mutex_delete(rs485_mutex);
        rs485_mutex = RT_NULL;
    }

    /* Reset statistics */
    rt_memset(&rs485_stats, 0, sizeof(rs485_stats));

    rt_kprintf("[RS485] Deinit success\n");
}

/**
  * @brief  Get RS485 communication statistics
  * @param  stats: Pointer to user statistics buffer
  * @retval None
  */
void rs485_get_stats(rs485_stats_t *stats)
{
    if (stats != RT_NULL)
        rt_memcpy(stats, &rs485_stats, sizeof(rs485_stats_t));
}
RTM_EXPORT(rs485_get_stats);

/**
  * @brief  Reset RS485 communication statistics
  * @param  None
  * @retval None
  */
void rs485_reset_stats(void)
{
    rt_memset(&rs485_stats, 0, sizeof(rs485_stats));
}
RTM_EXPORT(rs485_reset_stats);

/**
  * @brief  RS485 initialization function (DMA receive mode)
  * @param  None
  * @retval 0: Initialization successful
  *         -1: Initialization failed (error in resource creation/device open)
  * @note   Auto-initialized during RT-Thread startup (INIT_APP_EXPORT)
  */
rt_int32_t rs485_init(void)
{
    rt_err_t ret = RT_EOK;
    struct serial_configure uart_config = RT_SERIAL_CONFIG_DEFAULT; // Default UART config

    /* 1. Find RS485 UART device */
    rs485_uart = rt_device_find(RS485_UART_NAME);
    if (rs485_uart == RT_NULL)
    {
        rt_kprintf("[RS485 ERR] Failed to find UART device: %s\n", RS485_UART_NAME);
        return -1;
    }

    /* 2. Initialize semaphore for DMA receive notify */
    ret = rt_sem_init(&rx_sem_rs485, "rx_sem_rs485", 0, RT_IPC_FLAG_FIFO);
    if (ret != RT_EOK)
    {
        rt_kprintf("[RS485 ERR] Init semaphore failed! err: %d\n", ret);
        return -1;
    }

    /* 3. Create mutex to protect shared resources */
    rs485_mutex = rt_mutex_create("rs485_mutex", RT_IPC_FLAG_FIFO);
    if (rs485_mutex == RT_NULL)
    {
        rt_kprintf("[RS485 ERR] Failed to create mutex!\n");
        rt_sem_detach(&rx_sem_rs485);
        return -1;
    }

    /* 4. Open UART device (DMA RX mode + stream mode) */
    ret = rt_device_open(rs485_uart, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_STREAM);
    if (ret != RT_EOK)
    {
        rt_kprintf("[RS485 ERR] Failed to open UART device! ret=%d\n", ret);
        rs485_deinit();
        return -1;
    }

    /* 5. Configure custom UART parameters (override default) */
    uart_config.baud_rate = RS485_BAUD_RATE;
    uart_config.data_bits = RS485_DATA_BITS;
    uart_config.stop_bits = RS485_STOP_BITS;
    uart_config.parity = RS485_PARITY;
    ret = rt_device_control(rs485_uart, RT_DEVICE_CTRL_CONFIG, &uart_config);
    if (ret != RT_EOK)
    {
        rt_kprintf("[RS485 ERR] Config UART failed! err: %d\n", ret);
        rs485_deinit();
        return -1;
    }

    /* 6. Set DMA receive interrupt callback function */
    ret = rt_device_set_rx_indicate(rs485_uart, rs485_uart_input);
    if (ret != RT_EOK)
    {
        rt_kprintf("[RS485 ERR] Set rx indicate failed! err: %d\n", ret);
        rs485_deinit();
        return -1;
    }

    /* 7. Create and start RS485 receive thread */
    rt_thread_t rx_thread = rt_thread_create("rs485_rx",
                                             rs485_rx_thread,
                                             RT_NULL,
                                             RS485_THREAD_STACK,
                                             RS485_RX_THREAD_PRIO,
                                             10);
    if (rx_thread == RT_NULL)
    {
        rt_kprintf("[RS485 ERR] Failed to create RX thread!\n");
        rs485_deinit();
        return -1;
    }
    rt_thread_startup(rx_thread);

    // Print initialization success log
    rt_kprintf("[RS485 INFO] Initialization successful (UART=%s, Baud=%d, DMA RX)\n", RS485_UART_NAME, RS485_BAUD_RATE);
    return 0;
}

/* Auto-initialize during RT-Thread application startup */
INIT_COMPONENT_EXPORT(rs485_init);