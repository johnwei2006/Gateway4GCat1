#ifndef DEBUG_LOG_H_
#define DEBUG_LOG_H_

#include <rtthread.h>

#define DBG_LEVEL_NONE    0
#define DBG_LEVEL_ERROR   1
#define DBG_LEVEL_WARN    2
#define DBG_LEVEL_INFO    3
#define DBG_LEVEL_DEBUG   4

#ifndef DBG_LEVEL
#define DBG_LEVEL   DBG_LEVEL_INFO
#endif

#ifndef MODULE_NAME
#define MODULE_NAME  "UNKNOWN"
#endif

#if DBG_LEVEL >= DBG_LEVEL_ERROR
    #define LOG_E(fmt, ...)   rt_kprintf("[E][" MODULE_NAME "] " fmt, ##__VA_ARGS__)
#else
    #define LOG_E(fmt, ...)
#endif

#if DBG_LEVEL >= DBG_LEVEL_WARN
    #define LOG_W(fmt, ...)   rt_kprintf("[W][" MODULE_NAME "] " fmt, ##__VA_ARGS__)
#else
    #define LOG_W(fmt, ...)
#endif

#if DBG_LEVEL >= DBG_LEVEL_INFO
    #define LOG_I(fmt, ...)   rt_kprintf("[I][" MODULE_NAME "] " fmt, ##__VA_ARGS__)
#else
    #define LOG_I(fmt, ...)
#endif

#if DBG_LEVEL >= DBG_LEVEL_DEBUG
    #define LOG_D(fmt, ...)   rt_kprintf("[D][" MODULE_NAME "] " fmt, ##__VA_ARGS__)
#else
    #define LOG_D(fmt, ...)
#endif

#endif /* DEBUG_LOG_H_ */