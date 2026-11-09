#include "modbus_parser.h"
#include "crc16.h"
#include "rs485.h"

int modbus_request_parse(uint8_t *rx_buf, uint16_t rx_len, modbus_request_message_t *modbus_request_message){
    // 1. 入参合法性检查
    if (rx_buf == NULL || modbus_request_message == NULL)
    {
        rt_kprintf("modbus parameter error: rx_buf or modbus_request_message is NULL\n");
        return -1;
    }

    // 2. 功能码合法性预检查
    if (rx_len < 1)
    {
        rt_kprintf("message length error: empty buffer\n");
        return -2;
    }
    uint8_t function_code = rx_buf[1];
    /*
    if (function_code != MODBUS_FC_READ_COILS && function_code != MODBUS_FC_READ_HOLD_REGS &&
        function_code != MODBUS_FC_READ_INPUT_REGS && function_code != MODBUS_FC_WRITE_SINGLE_COIL &&
        function_code != MODBUS_FC_WRITE_SINGLE_REG)
    {
        rt_kprintf("unsupported function code: 0x%02X\n", function_code);
        return -4;
    }*/

    // 3. 按功能码检查报文长度合法性
    uint16_t expected_min_len = 0;
    switch (function_code)
    {
        case MODBUS_FC_READ_COILS:             // 01: 地址1+功能码1+起始地址2+数量2+CRC2 = 8字节
        case MODBUS_FC_READ_DISCRETE_INPUTS:   // 02: 同上
        case MODBUS_FC_READ_HOLD_REGS:         // 03: 同上
        case MODBUS_FC_READ_INPUT_REGS:        // 04: 同上
        case MODBUS_FC_WRITE_SINGLE_COIL:      // 05: 地址1+功能码1+线圈地址2+值2+CRC2 = 8字节
        case MODBUS_FC_WRITE_SINGLE_REG:       // 06: 地址1+功能码1+寄存器地址2+值2+CRC2 = 8字节
            expected_min_len = 8;
            break;
        default:
            expected_min_len = 8; // 兜底，与上面保持一致
            break;
    }

    if (rx_len != expected_min_len) // Modbus请求报文长度固定，不是"小于"而是"不等于"
    {
        rt_kprintf("message length error: fc=0x%02X, recv_len=%d, expected_len=%d\n", 
                    function_code, rx_len, expected_min_len);
        return -2;
    }

    // 4. CRC校验（低字节在前，高字节在后）
    uint16_t crc_recv = (rx_buf[rx_len-1] << 8) | rx_buf[rx_len-2]; // 原代码此处顺序正确
    uint16_t crc_calc = modbus_crc16(rx_buf, rx_len - 2);
    if (crc_recv != crc_calc)
    {
        rt_kprintf("CRC error: received=0x%04X, calculated=0x%04X\n", crc_recv, crc_calc);
        return -3;
    }

    // 5. 检查是否为异常响应（请求报文不会有此标志，防御性检查）
    if ((rx_buf[1] & 0x80) != 0)
    {
        uint8_t excp_code = rx_buf[2];
        rt_kprintf("Error response: function code=0x%02X, exception code=0x%02X\n", rx_buf[1], excp_code);
        return -5;
    }

    // 6. 按功能码解析报文内容
    modbus_request_message->slave_address = rx_buf[0];
    modbus_request_message->function_code = function_code;
    modbus_request_message->start_reg_address = (rx_buf[2] << 8) | rx_buf[3]; // 起始地址：高字节在前，低字节在后
    
    switch (function_code)
    {
        case MODBUS_FC_READ_COILS:
        case MODBUS_FC_READ_DISCRETE_INPUTS:
        case MODBUS_FC_READ_HOLD_REGS:
        case MODBUS_FC_READ_INPUT_REGS:
            // 读取类功能码：rx_buf[4][5] 为读取数量（高字节在前）
            modbus_request_message->reg_number = (rx_buf[4] << 8) | rx_buf[5];
            // 额外检查读取数量合法性（01/03/04最大读取数量分别为2000/125/125）
            if ((function_code == MODBUS_FC_READ_COILS && modbus_request_message->reg_number > 2000) ||
                ((function_code == MODBUS_FC_READ_DISCRETE_INPUTS || function_code == MODBUS_FC_READ_HOLD_REGS || function_code == MODBUS_FC_READ_INPUT_REGS) && modbus_request_message->reg_number > 125))
            {
                rt_kprintf("read count exceed limit: fc=0x%02X, count=%d\n", function_code, modbus_request_message->reg_number);
                return -6;
            }
            break;

        case MODBUS_FC_WRITE_SINGLE_COIL:
            // 写单个线圈：rx_buf[4][5] 为线圈值（0x0000=关，0xFF00=开，其他值非法）
            modbus_request_message->value = (rx_buf[4] << 8) | rx_buf[5];
            if (modbus_request_message->value != 0x0000 && modbus_request_message->value != 0xFF00)
            {
                rt_kprintf("invalid coil value: 0x%04X (must be 0x0000[off] or 0xFF00[on])\n", modbus_request_message->value);
                return -7;
            }
            break;

        case MODBUS_FC_WRITE_SINGLE_REG:
            // 写单个寄存器：rx_buf[4][5] 为写入值（无范围限制，直接解析）
            modbus_request_message->value = (rx_buf[4] << 8) | rx_buf[5];
            break;

        default:
            // 理论上不会走到这里，已提前检查功能码
            return -4;
    }

    // 7. 解析CRC值（低字节在前，高字节在后）
    modbus_request_message->crc = (rx_buf[6] << 8) | rx_buf[7]; // 固定8字节报文，CRC在6、7位

    rt_kprintf("modbus parse success: slave_address=0x%02X, function_code=0x%02X, start_addr=0x%04X, reg_number=0x%04X, value=0x%04X\n",
               modbus_request_message->slave_address,
               modbus_request_message->function_code,
               modbus_request_message->start_reg_address,
               modbus_request_message->reg_number,
               modbus_request_message->value);

    return 0;
}

/**
 * @brief  Modbus响应报文解析函数（支持01/03/04/05/06功能码）
 * @param  data: 接收的响应报文缓冲区
 * @param  len: 报文长度
 * @param  msg: 解析后的数据存储结构体（需确保value指针指向足够大的内存，或内部动态分配）
 * @retval 0: 解析成功; 负数: 解析失败（错误码）
 * @note   1. 解析01/03/04时，msg->value需提前分配内存（大小≥msg->byte_number）
 *         2. 解析05/06时，msg->value需至少分配2字节内存
 *         3. 异常响应时，msg->byte_number存储异常码，value为NULL
 */
int modbus_response_parse(uint8_t *data, uint16_t len, modbus_response_message_t *modbus_response_message)
{
    if (data == NULL || modbus_response_message == NULL || len < 5)
    {
        rt_kprintf("parse: param null or len too small\n");
        return -1;
    }

    // 清空响应结构体（关键！防止脏数据）
    memset(modbus_response_message, 0, sizeof(modbus_response_message_t));

    // 基础字段解析
    modbus_response_message->slave_address = data[0];
    modbus_response_message->function_code = data[1];
    modbus_response_message->crc = ((uint16_t)data[len-1] << 8) | data[len-2];

    // CRC 校验
    uint16_t crc_calc = modbus_crc16(data, len - 2);
    if (modbus_response_message->crc != crc_calc)
    {
        rt_kprintf("parse: CRC error! recv=0x%04X calc=0x%04X\n",
                   modbus_response_message->crc, crc_calc);
        return -3;
    }

    // 异常响应判断
    if ((modbus_response_message->function_code & MODBUS_EXCEPTION_FLAG) != 0)
    {
        uint8_t real_fc = modbus_response_message->function_code & ~0x80;
        uint8_t exc_code = data[2];
        modbus_response_message->byte_number = exc_code;

        rt_kprintf("parse: exception! fc=0x%02X code=0x%02X\n", real_fc, exc_code);
        return -6;
    }

    uint8_t fc = modbus_response_message->function_code;
    switch (fc)
    {
        //----------------------------------------------------------------------
        // 01: 读线圈
        //----------------------------------------------------------------------
        case MODBUS_FC_READ_COILS:
        {
            modbus_response_message->byte_number = data[2];
            uint16_t exp_len = 3 + modbus_response_message->byte_number + 2;
            if (len != exp_len) return -4;

            if (modbus_response_message->byte_number > MODBUS_VALUE_MAX_LENGTH)
                return -5;

            rt_memcpy(modbus_response_message->value, &data[3],
                      modbus_response_message->byte_number);
            break;
        }

        //----------------------------------------------------------------------
        // 02: 读离散输入
        //----------------------------------------------------------------------
        case MODBUS_FC_READ_DISCRETE_INPUTS:
        {
            modbus_response_message->byte_number = data[2];
            uint16_t exp_len = 3 + modbus_response_message->byte_number + 2;
            if (len != exp_len) return -4;

            if (modbus_response_message->byte_number > MODBUS_VALUE_MAX_LENGTH)
                return -5;

            rt_memcpy(modbus_response_message->value, &data[3],
                      modbus_response_message->byte_number);
            break;
        }

        //----------------------------------------------------------------------
        // 03: 读保持寄存器
        // 04: 读输入寄存器
        //----------------------------------------------------------------------
        case MODBUS_FC_READ_HOLD_REGS:
        case MODBUS_FC_READ_INPUT_REGS:
        {
            modbus_response_message->byte_number = data[2];
            uint16_t exp_len = 3 + modbus_response_message->byte_number + 2;
            if (len != exp_len) return -4;

            if (modbus_response_message->byte_number > MODBUS_VALUE_MAX_LENGTH)
                return -5;

            rt_memcpy(modbus_response_message->value, &data[3],
                      modbus_response_message->byte_number);
            break;
        }

        //----------------------------------------------------------------------
        // 05: 写单个线圈
        //----------------------------------------------------------------------
        case MODBUS_FC_WRITE_SINGLE_COIL:
        {
            if (len != 8) return -4;
            // 响应返回：地址 + 05 + 线圈地址 + 写入的线圈值 + CRC
            modbus_response_message->byte_number = 0;
            modbus_response_message->start_reg_address = data[2] << 8 | data[3];
            rt_memcpy(modbus_response_message->value, &data[4], 2);
            break;
        }

        //----------------------------------------------------------------------
        // 06: 写单个寄存器
        //----------------------------------------------------------------------
        case MODBUS_FC_WRITE_SINGLE_REG:
        {
            if (len != 8) return -4;
            // 响应返回：地址 + 06 + 寄存器地址 + 写入的寄存器值 + CRC
            modbus_response_message->byte_number = 0;
            modbus_response_message->start_reg_address = data[2] << 8 | data[3];
            rt_memcpy(modbus_response_message->value, &data[4], 2);
            break;
        }

        //----------------------------------------------------------------------
        // 15 (0x0F): 写多个线圈
        //----------------------------------------------------------------------
        case MODBUS_FC_WRITE_MULTIPLE_COILS:
        {
            if (len != 8) return -4;
            // 响应返回：地址 + 0F + 起始地址 + 写入的线圈数量 + CRC
            modbus_response_message->start_reg_address = data[2] << 8 | data[3];
            rt_memcpy(modbus_response_message->value, &data[4], 2);
            break;
        }

        //----------------------------------------------------------------------
        // 16 (0x10): 写多个寄存器
        //----------------------------------------------------------------------
        case MODBUS_FC_WRITE_MULTIPLE_REGS:
        {
            if (len != 8) return -4;
            // 响应返回：地址 + 10 + 起始地址 + 寄存器数量 + CRC
            modbus_response_message->start_reg_address = data[2] << 8 | data[3];
            rt_memcpy(modbus_response_message->value, &data[4], 2);
            break;
        }

        default:
            rt_kprintf("parse: unsupported fc=0x%02X\n", fc);
            return -8;
    }

    rt_kprintf("parse response success! slave_address=0x%02X, function_code=0x%02X, start_addr=0x%02X bytes=%d\n",
               modbus_response_message->slave_address,
               modbus_response_message->function_code,
               modbus_response_message->start_reg_address,
               modbus_response_message->byte_number);
    return 0;
}

/**
 * @brief  Modbus CRC16 校验函数（核心，必须）
 * @param  pdata: 待校验的报文数据
 * @param  len: 数据长度（不含 CRC 字节）
 * @return 计算出的 CRC16 校验值（低字节在前，高字节在后）
 */
uint16_t modbus_crc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc = crc >> 1;
            }
        }
    }
    return crc;
};