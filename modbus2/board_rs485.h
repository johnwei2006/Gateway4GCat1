#ifndef __BOARD_RS485_H
#define __BOARD_RS485_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include <rtdef.h>

#define     BD_485      9

#define     MB_READ_HOLDINGREG    3		// 读保持寄存器
#define     MB_READ_INPUTREG      4		// 读输入寄存器
#define     MB_WRITE_SINGLEREG    6		// 写保持寄存器

#define RS485_POWER_EN_Pin GPIO_PIN_9
#define RS485_POWER_EN_GPIO_Port GPIOB

#define RS485_DIR_Pin GPIO_PIN_12
#define RS485_DIR_GPIO_Port GPIOC

struct rs485_arg
{
    int addr:8;
    int func:4;
    int bufindex:16;
    int buflen:4;
};
 
extern UART_HandleTypeDef huart3;

void USART3_Init(void);

/*读输入寄存器或保持寄存器*/
bool modbus_read(uint8_t addr, uint8_t fun_num, uint16_t reg, uint16_t reg_cnt, uint16_t *read_buf, uint32_t timeout);
/*写单个保持寄存器*/
bool modbus_write(uint8_t addr, uint8_t fun_num, uint16_t reg, uint16_t reg_cnt, uint16_t *write_buf, uint32_t timeout);

/*RS485启动*/
void board_485_launch(void);
/*RS485低功耗处理*/
void board_485_lowpower(void);
/*采集485传感器数据*/
int board_485_collect(void *collect_buf, struct rs485_arg *arg, int timeout);

void _Error_Handler(char *s, int num);

#ifndef Error_Handler
#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
#endif

#endif