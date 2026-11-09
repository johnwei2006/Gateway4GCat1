#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "rtc_manager.h"
#include <time.h>

/* 定义 RTC 设备名称，根据实际硬件修改，一般为 "rtc0" */
#define RTC_DEVICE_NAME    "rtc"

static rt_device_t rtc_dev;  /* RTC 设备句柄 */

/**
 * @brief  初始化 RTC 设备
 * @return 0:成功，-1:失败
 */
int rtc_device_init(void)
{
    /* 查找 RTC 设备 */
    rtc_dev = rt_device_find(RTC_DEVICE_NAME);
    if (rtc_dev == RT_NULL)
    {
        rt_kprintf("Find RTC Device %s Failed!\n", RTC_DEVICE_NAME);
        return -1;
    }

    /* 打开 RTC 设备 */
    if (rt_device_open(rtc_dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
    {
        rt_kprintf("Open RTC Device %s Failed!\n", RTC_DEVICE_NAME);
        return -1;
    }

    rt_kprintf("RTC Device Init Success!\n");
    return 0;
}

/**
 * @brief 获取RTC时间
 * @param tm_out: struct tm指针，传出时间
 * @return RT_EOK成功
 */
int read_rtc_time(rt_device_rtc_time_t *rtc_time)
{
    rt_err_t ret;
    time_t timestamp = 0;

    /* 通过 control 接口读取 RTC 时间（time_t 格式） */
    ret = rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &timestamp);
    if (ret != RT_EOK)
    {
        rt_kprintf("Read RTC Failed!\n");
        return -1;
    }

    /* 将 time_t 转换为日期时间结构体 */
    struct tm tm_time;
    localtime_r(&timestamp, &tm_time);

    rtc_time->year  = tm_time.tm_year + 1900;
    rtc_time->month = tm_time.tm_mon + 1;
    rtc_time->day   = tm_time.tm_mday;
    rtc_time->hour  = tm_time.tm_hour;
    rtc_time->min   = tm_time.tm_min;
    rtc_time->sec   = tm_time.tm_sec;

    /* 打印格式化的 RTC 时间 */
    rt_kprintf("Current RTC Time: %04d-%02d-%02d %02d:%02d:%02d\n",
               rtc_time->year, rtc_time->month, rtc_time->day,
               rtc_time->hour, rtc_time->min, rtc_time->sec);

    return 0;
}
int set_rtc_time(rt_device_rtc_time_t *time)
{
    rt_err_t ret;

    if (rtc_dev == RT_NULL || time == RT_NULL)
    {
        return -1;
    }

    /* 将 rt_device_rtc_time_t 转换为 time_t */
    struct tm tm_time = {0};
    tm_time.tm_year = time->year - 1900;
    tm_time.tm_mon  = time->month - 1;
    tm_time.tm_mday = time->day;
    tm_time.tm_hour = time->hour;
    tm_time.tm_min  = time->min;
    tm_time.tm_sec  = time->sec;

    time_t timestamp = mktime(&tm_time);

    /* 设置 RTC 时间 */
    ret = rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_TIME, &timestamp);
    if (ret != RT_EOK)
    {
        rt_kprintf("Set RTC Failed!\n");
        return -1;
    }

    rt_kprintf("Set RTC Success!\n");
    return 0;
}

/**
 * @brief 打印RTC时间辅助函数
 */
void rtc_print_time(void)
{
    rt_device_rtc_time_t rtc_time = {0};
    read_rtc_time(&rtc_time);
    rt_kprintf("RTC Time: %04d-%02d-%02d %02d:%02d:%02d\n",
               rtc_time.year, rtc_time.month, rtc_time.day,
               rtc_time.hour, rtc_time.min, rtc_time.sec);
}
INIT_COMPONENT_EXPORT(rtc_device_init);