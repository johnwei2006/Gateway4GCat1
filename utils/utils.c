#include "utils.h"
#include "rtthread.h"
#include "rthw.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

int  max_common_divisor(int v1, int v2){
    int gt, lt, c;
    if(v1 > v2){
        gt = v1;
        lt = v2;
    } else {
        gt = v2;
        lt = v1;
    }
	while(lt)
	{
		c  = gt%lt;
		gt = lt;
		lt = c;
	}
    return gt;
};



void substr(const char *src, char *dest, int start, int length) {
   int i;
   for (i = 0; i < length && src[start + i] != '\0'; i++) {
      dest[i] = src[start + i];
   }
   dest[i] = '\0';
};

void to_upper(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (islower(str[i])) {
            str[i] = toupper(str[i]);
        }
    }
}

void to_lower(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (isupper(str[i])) {
            str[i] = tolower(str[i]);
        }
    }
}
uint32_t generate_rand_long(void) {
    static uint8_t initialized = 0;
    if (!initialized) {

        srand((unsigned int)time(NULL));
        initialized = 1;
    }
    uint32_t rand_high = (uint32_t)(rand() & 0xFFFF);  // 高16位
    uint32_t rand_low  = (uint32_t)(rand() & 0xFFFF);  // 低16位
    return (rand_high << 16) | rand_low;
}



uint16_t generate_rand_short(void){
   static uint8_t initialized = 0;
    if (!initialized) {
        srand((unsigned int)time(NULL));
        initialized = 1;
    }
    return (uint16_t)(rand() % 65535 + 1);
}

// 计算简单校验和
uint8_t calculate_checksum(const uint8_t *data, uint16_t len)
{
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        checksum ^= data[i];
    }
    return checksum;
}

void print_hex_string(char *data, rt_size_t len)
{
    if (data == RT_NULL || len == 0)
    {
        rt_kprintf("Invalid data or length\n");
        return;
    }

    rt_size_t i;
    for (i = 0; i < len; i++)
    {
        // 转换为 unsigned char 避免符号扩展，%02x 确保输出 2 位 16 进制
        rt_kprintf("%02x ", (unsigned char)data[i]);
        
        // 每 16 个字节换行，方便查看
        if ((i + 1) % 16 == 0)
        {
            rt_kprintf("\n");
        }
    }
    rt_kprintf("\n"); // 结束后换行
}

// 自定义整数转字符串函数（十进制）
char* cps_itoa(int value, char* str, int base) {
    if (base != 10) {  // 仅支持十进制（如需其他进制可扩展）
        *str = '\0';
        return str;
    }

    char* ptr = str;
    char* ptr1 = str;
    int num;
    int is_negative = 0;

    // 处理负数
    if (value < 0) {
        is_negative = 1;
        num = -value;  // 转为正数处理
    } else {
        num = value;
    }

    // 处理0的特殊情况
    if (num == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return str;
    }

    // 逐位转换数字为字符（逆序）
    while (num > 0) {
        *ptr++ = (num % 10) + '0';  // 取个位并转为字符
        num /= 10;
    }

    // 添加负号
    if (is_negative) {
        *ptr++ = '-';
    }

    *ptr = '\0';  // 字符串结束符

    // 反转字符串（因为上面的转换是逆序的）
    char temp;
    ptr--;  // 指向最后一个有效字符
    while (ptr1 < ptr) {
        temp = *ptr1;
        *ptr1++ = *ptr;
        *ptr-- = temp;
    }

    return str;  // 返回转换后的字符串
}

void remove_item_by_value(uint8_t items[], uint8_t item_size, uint8_t removed_item_value){
    if(items == RT_NULL){
        return;
    }
    for(uint8_t i = 0; i < item_size; i++){
        if(items[i] == removed_item_value){
            for(uint8_t j = i; j < item_size - 1; j++){
                items[j] = items[j + 1];
            }
            items[item_size - 1] = 0; // 可选：将最后一个元素设为0
            break; // 只移除第一个匹配项
        }
    }
};