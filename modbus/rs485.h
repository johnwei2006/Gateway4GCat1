/**
  ******************************************************************************
  * @file    rs485_max13487.h
  * @author  RT-Thread Development Team
  * @brief   RS485 driver header for STM32F407 with MAX13487 (Auto Transmit/Receive + DMA RX)
  * @note    MAX13487 supports automatic direction control, no GPIO control for DE/RE pin required
  ******************************************************************************
  */
#ifndef __RS485_H
#define __RS485_H


#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

/* ==================== Hardware Configuration Parameters ==================== */
#define RS485_UART_NAME         "uart3"        /* UART device name for RS485 communication */
#define RS485_BAUD_RATE         9600           /* UART baud rate (match OSU sensor default 9600) */
#define RS485_DATA_BITS         DATA_BITS_8    /* UART data bits configuration */
#define RS485_STOP_BITS         STOP_BITS_1    /* UART stop bits configuration */
#define RS485_PARITY            PARITY_NONE    /* UART parity bit configuration */
#define RS485_RX_BUF_SIZE       1024           /* RS485 receive buffer size (bytes) */
#define RS485_RX_THREAD_PRIO    12             /* RX thread priority (higher than TX for reliability) */
#define RS485_TEST_THREAD_PRIO  14             /* Test thread priority (optional) */
#define RS485_THREAD_STACK      768            /* RX thread stack size (bytes) */

/* ==================== DMA Frame Detection Configuration ==================== */
#define RS485_FRAME_TIMEOUT_MS  5              /* Frame timeout (3.5T char gap at 9600 baud + margin) */
#define RS485_DMA_STABILIZE_MS  1              /* DMA data stabilize delay before read (ms) */
#define RS485_BUS_TURNAROUND_MS 5              /* Bus turnaround extra delay after TX (ms) */

/* ==================== OSU PAR Sensor Modbus Configuration ==================== */
#define OSU_SENSOR_ADDR         0x13           /* OSU sensor slave address (hex) */
#define OSU_REG_ADDR            0x0000         /* PAR register address (0x0000 for OSU sensor) */
#define OSU_REG_CNT             0x0001         /* Number of registers to read (1 register = 2 bytes) */
#define OSU_READ_CMD_LEN        8              /* Length of Modbus read command (bytes) */
/* Modbus RTU command: 13 03 00 00 00 01 87 78 (binary) */
static const rt_uint8_t osu_read_cmd[OSU_READ_CMD_LEN] = {0x01,0x05,0x00,0x00,0xFF,0x00,0x8C,0x3A};

/* ==================== Statistics Structure ==================== */
typedef struct {
    rt_uint32_t rx_frames;          /* Complete frames received */
    rt_uint32_t rx_bytes;           /* Total RX bytes */
    rt_uint32_t tx_frames;          /* Frames sent */
    rt_uint32_t tx_bytes;           /* Total TX bytes */
    rt_uint32_t rx_overflows;       /* Buffer overflow count */
    rt_uint32_t frame_timeouts;     /* Frame timeout triggers */
    rt_uint32_t dma_errors;         /* DMA error count */
} rs485_stats_t;

/* ==================== Global Function Declarations ==================== */
rt_int32_t rs485_read(rt_uint8_t *data, rt_uint32_t len);
rt_int32_t rs485_send(rt_uint8_t *data, rt_uint32_t len);
rt_int32_t rs485_init(void);
void rs485_deinit(void);
void rs485_get_stats(rs485_stats_t *stats);
void rs485_reset_stats(void);

#endif /* __RS485_H */
