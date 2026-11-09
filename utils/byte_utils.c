#include "byte_utils.h"

static uint8_t hex_lookup[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
'8','9', 'a', 'b', 'c', 'd', 'e', 'f' };
void array2hex(uint8_t* in, int length, uint8_t* out)
{
    int i;
    for (i = 0; i < length; ++i)
    {
        out[2 * i] = hex_lookup[in[i] >> 4];
        out[2 * i + 1] = hex_lookup[in[i] & 0x0f];
    }
}

rt_err_t hex_str_to_bytes(uint8_t *hex_str, uint8_t *byte_buf, uint16_t *buf_len)
{
    if (hex_str == RT_NULL || byte_buf == RT_NULL || buf_len == RT_NULL)
    {
        rt_kprintf("[RS485] hex convert err: null parameter\n");
        return -RT_EINVAL;
    }

    uint16_t str_len = rt_strlen((char*)hex_str);
    if (str_len % 2 != 0)
    {
        rt_kprintf("[RS485] hex convert err: odd length(%d)\n", str_len);
        return -RT_EINVAL;
    }

    *buf_len = str_len / 2;
    for (uint16_t i = 0; i < str_len; i += 2)
    {
        char high = toupper(hex_str[i]);
        char low = toupper(hex_str[i+1]);

        uint8_t high_val = 0, low_val = 0;
        if (high >= '0' && high <= '9') high_val = high - '0';
        else if (high >= 'A' && high <= 'F') high_val = high - 'A' + 10;
        else return -RT_EINVAL;
        if (low >= '0' && low <= '9') low_val = low - '0';
        else if (low >= 'A' && low <= 'F') low_val = low - 'A' + 10;
        else return -RT_EINVAL;
        byte_buf[i/2] = (high_val << 4) | low_val;
    }
    return RT_EOK;
}