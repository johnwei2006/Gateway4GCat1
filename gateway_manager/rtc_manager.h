
#ifndef RTC_MANAGER_H_
#define RTC_MANAGER_H_
typedef struct rt_device_rtc_time
{
    uint16_t year;  // 改为 uint16_t，原是 uint8_t
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
} rt_device_rtc_time_t;

int rtc_device_init(void);
int read_rtc_time(rt_device_rtc_time_t *rtc_time);
int set_rtc_time(rt_device_rtc_time_t *time);
void rtc_print_time(void);

#endif /*RTC_MANAGER_H_*/
