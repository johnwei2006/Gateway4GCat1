#ifndef __MODBUS_PARSER_H
#define __MODBUS_PARSER_H
#include <rtthread.h>
#include "../utils/utils.h"
#include "./rs485.h"

// Modbus功能码定义
#define MODBUS_FC_READ_COILS              0x01    // 读线圈
#define MODBUS_FC_READ_DISCRETE_INPUTS    0x02    // 读离散输入点
#define MODBUS_FC_READ_HOLD_REGS          0x03    // 读保持寄存器
#define MODBUS_FC_READ_INPUT_REGS         0x04    // 读输入寄存器
#define MODBUS_FC_WRITE_SINGLE_COIL       0x05    // 写单个线圈
#define MODBUS_FC_WRITE_SINGLE_REG        0x06    // 写单个保持寄存器
#define MODBUS_FC_WRITE_MULTIPLE_COILS    0x0F    // 写多个线圈
#define MODBUS_FC_WRITE_MULTIPLE_REGS     0x10    // 写多个保持寄存器
#define MODBUS_EXCEPTION_FLAG             0x80    // 异常响应标志位

// Modbus 异常码定义
#define MODBUS_EXCP_ILLEGAL_FUNC     0x01    // 非法功能码
#define MODBUS_EXCP_ILLEGAL_ADDR     0x02    // 非法寄存器地址
#define MODBUS_EXCP_ILLEGAL_VALUE    0x03    // 非法寄存器数量/值
#define MODBUS_EXCP_SLAVE_FAILURE    0x04    // 从站设备故障

#define MODBUS_VALUE_MAX_LENGTH      32      // 最大值长度，根据功能码不同而不同
typedef struct {
  uint8_t   slave_address;       // 从站地址
  uint8_t   function_code;       // 功能码
  uint16_t  start_reg_address;   // 线圈起始地址（功能码为01/02/05时）或寄存器起始地址（功能码为03/06时）
  uint16_t  reg_number;          // 线圈数量（function_code = 01、02）、寄存器数量（function_code = 03、04）
  uint16_t  value;               // 写入值（function_code = 05、06时使用，其他功能码此字段无效）
  uint16_t  crc;                 // CRC校验值（低字节在前）
} modbus_request_message_t;

typedef struct {
  uint8_t   slave_address;       // 从站地址
  uint8_t   function_code;       // 功能码
  uint8_t   byte_number;         // 字节数量（function_code = 01、02、03、04）
  uint16_t  start_reg_address;   // 线圈起始地址（功能码为05时）或寄存器起始地址（功能码为06时）
  uint8_t   value[MODBUS_VALUE_MAX_LENGTH]; // 线圈值(function_code = 01、02),寄存器值（function_code = 03、04）、修改的值（function_code = 05、06）
  uint16_t  crc;                 // CRC校验值（低字节在前）
} modbus_response_message_t;
int modbus_request_parse(uint8_t *data, uint16_t len, modbus_request_message_t *modbus_request_message);
int modbus_response_parse(uint8_t *data, uint16_t len, modbus_response_message_t *modbus_response_message);
uint16_t modbus_crc16(const uint8_t *data, uint32_t len);
#endif

