#include "time_utils.h"
#include <rtthread.h>

const char *get_localtime(void)
{
   static char timeBuffer[16];
   time_t now = time(NULL);
   struct tm *tim = localtime(&now);
   rt_sprintf(timeBuffer,"%02u:%02u:%02u\0",tim->tm_hour,tim->tm_min,tim->tm_sec);
   return (char *)timeBuffer;
} 

/**
 * 获取当前时间对应的6位16进制字符串
 * 返回值：动态分配的字符串（格式如"0A141E"），需用free()释放
 * 失败时返回NULL
 */
 rt_err_t get_current_time_hex(char* time_hex) {
    // 分配字符串空间：6位16进制 + 终止符'\0'，共7字节
    //char* hex_str = (char*)rt_malloc(7 * sizeof(char));
    if (time_hex == RT_NULL || strlen(time_hex) < 7) {
        return RT_EINVAL;  
    }

    time_t rawtime;
    struct tm* local_time;
    unsigned int hex_code;

    // 获取时间戳
    if (time(&rawtime) == (time_t)-1) {
        return RT_EINVAL;  // 时间获取失败
    }

    // 转换为本地时间
    local_time = localtime(&rawtime);
    if (local_time == RT_NULL) {
        return RT_EINVAL;  // 本地时间转换失败
    }

    // 提取时、分、秒（确保在有效范围）
    int hour = local_time->tm_hour;   // 0-23
    int minute = local_time->tm_min;  // 0-59
    int second = local_time->tm_sec;  // 0-59

    // 组合为24位数值（时占高8位，分中8位，秒低8位）
    hex_code = (hour << 16) | (minute << 8) | second;

    // 格式化为6位16进制字符串（大写，不足补0）
    snprintf(time_hex, 7, "%06X", hex_code);  // 限制写入7字节（含'\0'）

    return RT_EOK;
}

const char *get_current_time_stamp(void){
    static char timeBuffer[16];  // 格式：HH:MM:SS（8字节+终止符，16字节足够）
    time_t now;
    struct tm tim;  // 栈上的tm结构体（线程安全）

    // 获取当前时间（线程安全）
    now = time(NULL);
    localtime_r(&now, &tim);  // 用localtime_r替代localtime（线程安全）

    // 格式化时间（确保格式正确，无多余转义）
    rt_sprintf(timeBuffer, "%02d:%02d:%02d", 
               tim.tm_hour, tim.tm_min, tim.tm_sec);  // 用%d更安全（tm结构体成员是int）

    return timeBuffer;
};