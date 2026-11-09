/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-08-08     John       the first version
 */
#ifndef APPLICATIONS_LED_H_
#define APPLICATIONS_LED_H_

#include <board.h>
#include <rtdevice.h>

#define LED_PIN_BLUE_LORA_1  GET_PIN(C, 1)
#define LED_PIN_GREEN_SYS_2  GET_PIN(C, 2)
//#define LED_PIN_RED_3   GET_PIN(B, 2)

void turn_on_green_led(void);
void turn_off_green_led(void);
void turn_on_blue_led(void);
void turn_off_blue_led(void);
void turn_on_red_led(void);
void turn_off_red_led(void);
#endif /* APPLICATIONS_LED_H_ */
