/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-08-08     John       the first version
 */

#include <led.h>
#include <rtdbg.h>

void turn_on_green_led(void){
    rt_pin_write(LED_PIN_GREEN_SYS_2, PIN_LOW);
    //rt_kprintf("turn on green led!\n");
};
void turn_off_green_led(void){
    rt_pin_write(LED_PIN_GREEN_SYS_2, PIN_HIGH);
    //rt_kprintf("turn off green led!\n");
};
void turn_on_blue_led(void){
    rt_pin_write(LED_PIN_BLUE_LORA_1, PIN_LOW);
    //rt_kprintf("turn on blue led!\n");
};
void turn_off_blue_led(void){
    rt_pin_write(LED_PIN_BLUE_LORA_1, PIN_HIGH);
    //rt_kprintf("turn off blue led!\n");
};
