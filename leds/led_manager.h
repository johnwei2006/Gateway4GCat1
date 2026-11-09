#ifndef APPLICATIONS_LED_MANAGER_H_
#define APPLICATIONS_LED_MANAGER_H_

#include <stdbool.h>
#include "../utils/cps_list.h"
#include "abstract_led.h"

static bool is_initialized = false;
static struct cps_list_node *led_head_node;

void manager_led_add(led_property_t* led_property); 
void manager_led_turn_on(char* led_name);
void manager_led_turn_off(char* led_name);
void manager_led_reset_period(char* led_name, int period_on, int period_off);
static led_t* query_led_by_name(char* p_led_name);
#endif /*APPLICATIONS_LED_MANAGER_H_*/
