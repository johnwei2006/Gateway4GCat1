#ifndef TIME_UTILS_H_
#define TIME_UTILS_H_
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <rtdef.h>

const char *get_localtime(void);
rt_err_t get_current_time_hex(char* time_hex);
const char *get_current_time_stamp(void);
#endif /*TIME_UTILS_H_*/
