#include "board_rs485.h"
#include "modbus_crc16.h"
#include <rtthread.h>

UART_HandleTypeDef huart3;

//#define DEBUG
static bool modbus_send(uint8_t *buf, uint16_t buf_len, uint32_t timeout)
{
    HAL_GPIO_WritePin(RS485_DIR_GPIO_Port, RS485_DIR_Pin, GPIO_PIN_SET);
    HAL_UART_Transmit(&huart3, buf, buf_len, timeout);
    return true;
}

static bool modbus_recv(uint8_t *buf, uint16_t buf_len, uint32_t timeout)
{
    bool result = false;
    __HAL_UART_CLEAR_NEFLAG(&huart3);
    HAL_GPIO_WritePin(RS485_DIR_GPIO_Port, RS485_DIR_Pin, GPIO_PIN_RESET);
    if(HAL_OK == HAL_UART_Receive(&huart3, buf, buf_len, timeout))
    {
        result = true;
    }

    return result;
}

#define DEBUG
/*读输入寄存器或保持寄存器*/
bool modbus_read(uint8_t addr, uint8_t fun_num, uint16_t reg, uint16_t reg_cnt, uint16_t *read_buf, uint32_t timeout)
{
    uint8_t send_buf[8] = {0}, recv_buf[120] = {0};
    uint16_t send_buf_len = 0;
    uint16_t recv_buf_len = reg_cnt * 2 + 5;
    uint16_t send_crc = 0, recv_crc_chack = 0;
    bool result = false;

    if(fun_num != MB_READ_HOLDINGREG && fun_num != MB_READ_INPUTREG)
    {
        return false;
    }
    
    send_buf[0] = addr;
    send_buf[1] = fun_num;
    send_buf[2] = (uint8_t)(reg >> 8);
    send_buf[3] = (uint8_t)reg;
    send_buf[4] = (uint8_t)(reg_cnt >> 8);
    send_buf[5] = (uint8_t)reg_cnt;
    send_buf_len = 6;
    send_crc = CRC16((const char *)send_buf, send_buf_len);
    send_buf[6] = (uint8_t)send_crc;
    send_buf[7] = (uint8_t)(send_crc >> 8);
    send_buf_len = 8;

    #ifdef DEBUG
    rt_kprintf("RS485 Tx :");
    for(int i=0;i<send_buf_len; i++)
    {
        rt_kprintf("%02X ", send_buf[i]);
    }
    rt_kprintf("\r\n");
    #endif
    if(true == modbus_send(send_buf, send_buf_len, timeout))
    {
        if(true == modbus_recv(recv_buf, recv_buf_len, timeout))
        {

            recv_crc_chack = modbus_crc_16(send_crc, recv_buf, recv_buf_len);


            #ifdef DEBUG
            rt_kprintf("RS485 Rx :");
            for(int i=0;i<recv_buf_len; i++)
            {
                rt_kprintf("%02X ", recv_buf[i]);
            }
            rt_kprintf("\r\n");
            #endif
            if(recv_crc_chack == 0)
            {
                /*地址、功能码要一致、接收到的寄存器数据的个数一定是寄存器数量的两倍*/
                if(recv_buf[0] == addr && recv_buf[1] == fun_num \
                        && (recv_buf[2] == reg_cnt * 2))
                {
                    for(uint16_t i = 0; i < reg_cnt; i++)
                    {
                        read_buf[i] = recv_buf[3 + i * 2] * 256 + recv_buf[4 + i * 2];
                    }

                    result = true;
                }
            }
            else
            {
                #ifdef DEBUG
                rt_kprintf("RS485 Rx :CRC ERROR\r\n");
                #endif
            }
        }
        else
        {
            #ifdef DEBUG
            rt_kprintf("RS485 Rx :Timeout\r\n");
            #endif
        }
    }

    return result;
}

/*写保持寄存器*/
bool modbus_write(uint8_t addr, uint8_t fun_num, uint16_t reg, uint16_t reg_cnt, uint16_t *write_buf, uint32_t timeout)
{
    uint8_t send_buf[120] = {0}, recv_buf[8] = {0};
    uint16_t send_buf_len = 0;
    uint16_t recv_buf_len = 8;
    uint16_t send_crc = 0, recv_crc_chack = 0;
    bool result = false;

    if(fun_num != MB_WRITE_SINGLEREG)
    {
        return false;
    }
    send_buf[0] = addr;
    send_buf[1] = fun_num;
    send_buf[2] = (uint8_t)(reg >> 8);
    send_buf[3] = (uint8_t)reg;
    send_buf[4] = (uint8_t)(reg_cnt >> 8);
    send_buf[5] = (uint8_t)reg_cnt;
    send_buf_len = 6;

    for(uint16_t i = 0; i < reg_cnt; i++)
    {
        send_buf[6 + i * 2] = (uint8_t)(write_buf[i] >> 8);
        send_buf[6 + i * 2 + 1] = (uint8_t)write_buf[i];
    }

    send_buf_len += reg_cnt * 2;
    send_crc = CRC16((const char *)send_buf, send_buf_len);
    send_buf[send_buf_len++] = (uint8_t)send_crc;
    send_buf[send_buf_len++] = (uint8_t)(send_crc >> 8);

    if(true == modbus_send(send_buf, send_buf_len, timeout))
    {
        if(true == modbus_recv(recv_buf, recv_buf_len, timeout))
        {
            recv_crc_chack = modbus_crc_16(send_crc, recv_buf, recv_buf_len);

            if(recv_crc_chack == 0)
            {
                /*地址、功能码要一致、接收到的寄存器地址、数量一致*/
                if(recv_buf[0] == send_buf[0] && recv_buf[1] == send_buf[1] \
                        && recv_buf[2] == send_buf[2] && recv_buf[3] == send_buf[3] \
                        && recv_buf[4] == send_buf[4] && recv_buf[5] == send_buf[5])
                {
                    result = true;
                }
            }
        }
    }

    return result;
}

/*RS485启动*/
void board_485_launch(void)
{
    rt_thread_delay(10);
    __disable_irq();
    if(HAL_UART_Init(&huart3) != HAL_OK)
    {
        Error_Handler();
    }
    __enable_irq();
    
//    HAL_GPIO_Init(RS485_RDE_GPIO_Port, RS485_RDE_Pin);
    rt_thread_delay(10);
}

/*RS485低功耗处理*/
void board_485_lowpower(void)
{
    __disable_irq();
    if(HAL_UART_DeInit(&huart3) != HAL_OK)
    {
        Error_Handler();
    }
    __enable_irq();
    rt_thread_delay(10);
}

/*采集485传感器数据*/
int board_485_collect(void *collect_buf, struct rs485_arg *arg, int timeout)
{
    board_485_launch();
    int result = modbus_read(arg->addr, arg->func, arg->bufindex, arg->buflen, collect_buf, timeout);
    board_485_lowpower();
    return result;
}


void USART3_Init(void)
{

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }

}