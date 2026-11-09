#include "led_manager.h"

void manager_led_add(led_property_t* led_property){
    if(!is_initialized){
        led_head_node = (struct cps_list_node *)cps_list_init();
        is_initialized = true;
    }
    led_t* led = led_create(led_property);
    cps_list_add(&led_head_node, led);
}; 

void manager_led_turn_on(char* led_name){
    led_t* led = query_led_by_name(led_name);
    if(led == NULL){
        return;
    }
    led->turn_on();
    led_set_run_status(led, true);
};

void manager_led_turn_off(char* led_name){
    led_t* led = query_led_by_name(led_name);
    if(led == NULL){
        return;
    }
    led->turn_off();
    led_set_run_status(led, false);
};

void manager_led_reset_period(char* led_name, int period_on, int period_off){
    led_t* led = query_led_by_name(led_name);
    if(led == NULL){
        rt_kprintf("[LED] Error: LED not found: %s\n", led_name);  // 添加这行
        return;
    }
    rt_kprintf("[LED] Found LED: %s, old period: %d/%d\n", led_name, 
               led->config->led_period_on, led->config->led_period_off);  // 添加这行
    led_reset_period(led, period_on, period_off);
};

static led_t* query_led_by_name(char* p_led_name) {
    uint8_t size = cps_list_size(led_head_node);
    uint8_t i;
    for(i = 1; i <= size; i++){
        led_t* led = (led_t*)cps_list_get_node(led_head_node, i);
        if(led != NULL && strcmp(led->config->led_name, p_led_name) == 0) {
            return led;
        }
    }
    return NULL;
};
