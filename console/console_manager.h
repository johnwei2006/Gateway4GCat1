#ifndef __CONSOLE_MANAGER_H__
#define __CONSOLE_MANAGER_H__

#include "console.h"

#define COMMAND_KEY_MAX_LENGTH       255
#define COMMAND_VALUE_MAX_LENGTH     1024
extern rt_thread_t  console_manager_thread;

void console_manager_init(void);
void console_manager_thread_entry(void *parameter);
void print_usage(void);
#endif /*_CONSOLE_MANAGER_H_*/