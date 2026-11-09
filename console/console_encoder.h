#ifndef __CONSOLE_ENCODER_H__
#define __CONSOLE_ENCODER_H__

#include "console.h"

rt_bool_t console_decode(console_data_t    *console_data,    console_command_t *console_command);
rt_bool_t console_encode(console_command_t *console_command, console_data_t    *console_data);

#endif /*CONSOLE_ENCODER_H_*/
