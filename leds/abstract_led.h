#ifndef APPLICATIONS_ABSTRACT_LED_H_
#define APPLICATIONS_ABSTRACT_LED_H_

#include <stdbool.h>
#include <rtthread.h>

extern rt_mutex_t mutex_led;

typedef enum {
    led_mode_stop  = 0,   /*led turn off always */
    led_mode_run   = 1,   /*led flicker with certain on/off period ratio*/
} led_mode_t;

typedef enum {
    led_status_off  = 0,
    led_status_on   = 1,
} led_status_t;

typedef struct
{
    uint8_t         led_name[16];
		int             led_period_on;                  /* ms, when the value is -1, under led_run mode, the led will always turn on whatever value is 'led_period_off' */
    int             led_period_off;     
    int             led_period_delay;               /* ms delay to turn on the led, for future support */

    int             led_max_common_divisor;         /* max common devisor of led_period_on and led_period_off*/
    int             led_status_on_timeout_count;    /* = led_period_on /led_max_common_divisor */
    int             led_status_off_timeout_count;   /* = led_period_off/led_max_common_divisor */
} led_config_t;

typedef struct
{
    led_config_t*   config;
    rt_timer_t      timer;
    led_mode_t      mode;
		led_status_t    current_status;
    int             current_timeout_count;      /*timeout count in current status */
    void            (*turn_on)();
    void            (*turn_off)();
} led_t;

typedef struct
{
    char            led_name[16];
		int             led_period_on;                  /* ms, when the value is -1, under led_run mode, the led will always turn on whatever value is 'led_period_off' */
    int             led_period_off;     
    void            (*led_turn_on)();
    void            (*led_turn_off)();    
} led_property_t;

void    abstract_led_init(void);
void    led_set_run_status(led_t* led, bool p_status);
void    led_reset_period(led_t* led, int period_on, int period_off);
led_t*  led_create(led_property_t*);
void    led_timeout_handle(led_t* led);
#endif /*APPLICATIONS_ABSTRACT_LED_H_*/