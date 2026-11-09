//#ifndef CPS_UTILS_H_
//#define CPS_UTILS_H_
#ifndef UTILS_H_
#define UTILS_H_
#include <stdio.h>
#include <stdlib.h>
#include <rtthread.h>
#include <time.h>

int       max_common_divisor(int v1, int v2);
uint32_t  generate_rand_long(void);
void      substr(const char *src, char *dest, int start, int length);
void 			to_upper(char *str);
void 			to_lower(char *str);
uint16_t 	generate_rand_short(void);
uint8_t 	calculate_checksum(const uint8_t *data, uint16_t len);
void 			print_hex_string(char *data, rt_size_t len);
char* 		cps_itoa(int value, char* str, int base);
void remove_item_by_value(uint8_t items[], uint8_t item_size, uint8_t removed_item_value);
#if LOG_CLOUD_USE_COLOR
    #define LOG_CLOUD(...)       do { \
        rt_kprintf("\033[1;34;40m[%s]CLOUD:", get_current_time_stamp()); \
        rt_kprintf(__VA_ARGS__); \
        rt_kprintf("\r\n\033[0m"); \
    } while(0)
#else
    #define LOG_CLOUD(...)       do { \
        rt_kprintf("[%s]CLOUD:", get_current_time_stamp()); \
        rt_kprintf(__VA_ARGS__); \
        rt_kprintf("\r\n"); \
    } while(0)
#endif    
#endif /*UTILS_H_*/
