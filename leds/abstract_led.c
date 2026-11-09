#include "abstract_led.h"
#include "../utils/utils.h"

rt_mutex_t mutex_led = RT_NULL;

void abstract_led_init(){
    mutex_led = rt_mutex_create("mutex_led", RT_IPC_FLAG_FIFO);
}

led_t* led_create(led_property_t* led_property){
    led_config_t *led_config = (led_config_t*)malloc(sizeof(led_config_t));
    /*initialize the configuration of led*/
    
    strcpy(led_config->led_name, led_property->led_name);
    led_config->led_period_on                 = led_property->led_period_on;
    led_config->led_period_off                = led_property->led_period_off;

    led_config->led_max_common_divisor        = max_common_divisor(led_config->led_period_on, led_config->led_period_off);
    led_config->led_status_on_timeout_count   = led_config->led_period_on/led_config->led_max_common_divisor;
    led_config->led_status_off_timeout_count  = led_config->led_period_off/led_config->led_max_common_divisor;

    /*initialize the led*/
    led_t* led                            = (led_t*)malloc(sizeof(led_t));
    led->config                           = led_config;
    led->current_status                   = led_status_off;
    led->current_timeout_count            = 0;
    led->turn_on                          = led_property->led_turn_on;
    led->turn_off                         = led_property->led_turn_off;

    led->turn_off();
    /* creat and start timer for the led */
    rt_tick_t led_timeout = (1000 * led_config->led_max_common_divisor) / RT_TICK_PER_SECOND;
    led->timer = rt_timer_create(led_config->led_name, led_timeout_handle,
                             led,  (1000 * led_config->led_max_common_divisor)/RT_TICK_PER_SECOND,
                             RT_TIMER_FLAG_PERIODIC);
    if (led->timer == RT_NULL)
    {
        rt_kprintf("Led timer %s init err.\n", led_config->led_name);
        return NULL;
    }
    rt_timer_start(led->timer);
    return led;
};

/* this will be triggered in each led_max_common_divisor period */
void led_timeout_handle(led_t* led){
    rt_mutex_take(mutex_led, RT_WAITING_FOREVER);  // 添加互斥锁保护

    led->current_timeout_count++;
    
    if(led->current_status == led_status_on){
        if(led->current_timeout_count >= led->config->led_status_on_timeout_count){
            led->turn_off();
            led->current_status = led_status_off;
            led->current_timeout_count = 0;
        }
    } else {
        if(led->current_timeout_count >= led->config->led_status_off_timeout_count){
            led->turn_on();
            led->current_status = led_status_on;
            led->current_timeout_count = 0;
        }
    }
    
    rt_mutex_release(mutex_led);
};

void led_reset_period(led_t* led, int period_on, int period_off){
    // 添加线程保护
    rt_mutex_take(mutex_led, RT_WAITING_FOREVER);
    
    led_config_t *led_config = led->config;

    /*initialize the configuration of led*/
    led_config->led_period_on                 = period_on;
    led_config->led_period_off                = period_off;

    led_config->led_max_common_divisor        = max_common_divisor(led_config->led_period_on, led_config->led_period_off);
    led_config->led_status_on_timeout_count   = led_config->led_period_on/led_config->led_max_common_divisor;
    led_config->led_status_off_timeout_count  = led_config->led_period_off/led_config->led_max_common_divisor;

    rt_kprintf("[LED] New period: on=%d, off=%d, gcd=%d, on_count=%d, off_count=%d\n",
               period_on, period_off, led_config->led_max_common_divisor,
               led_config->led_status_on_timeout_count, 
               led_config->led_status_off_timeout_count); 

    led->config                           = led_config;
    
    /* 不强制重置LED状态，保持当前状态连续性 */
    // led->current_status                   = led_status_off;
    led->current_timeout_count            = 0;

    /* 使用 rt_timer_control 修改定时器周期，而不是重建定时器 */
    rt_timer_stop(led->timer);
    
    rt_tick_t new_period = (1000 * led_config->led_max_common_divisor) / RT_TICK_PER_SECOND;
    rt_kprintf("[LED] Timer period ticks: %d\n", new_period);

    rt_timer_control(led->timer, RT_TIMER_CTRL_SET_TIME, &new_period);
    
    rt_timer_start(led->timer);
    
    rt_mutex_release(mutex_led);
};

void led_set_run_status(led_t* led, bool p_status){
    led->current_status = p_status;
}

INIT_APP_EXPORT(abstract_led_init);