/*
 * Copyright (c) 2026-2027, KebAI CPS Development Team
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <stdint.h>
#include <rtdbg.h>
#include <stdlib.h>
#include "led.h"
#include "../leds/led_manager.h"
#include "../utils/byte_utils.h"
#include "../flash_internal/abstract_flash_internal.h"
#include "../flash_external/abstract_flash_external.h"
#include "../flash_external/flash_block.h"
#include "../console/console.h"
#include "../console/console_message_queue.h"
#include "../console/console_manager.h"
#include "../config/config_manager.h"
#include "../lora/lora_service.h"
#include "../lte/lte_app.h"
#include "../service_manager/service_manager_lora.h"
#include "../service_manager/service_manager_udp.h"
#include "../power_manager/power_manager.h"
#include "../modbus/rs485.h"
#include "../transport_lora/lora_handler.h"
#include "../transport_udp/business_data_cache.h"
#include "../transport_udp/transport_udp_handler.h"

void led_init(void);
void connect_callback(lte_state_t lte_status);
int main(void)
{	
    rt_kprintf("KB_Gateway4GCat1 build time:%s\n", __DATE__ " " __TIME__);
	  power_manager_init();
    led_init();
		// upd startup
    //udp_app_init("39.107.77.230", 50001);
	  udp_app_init("\"www.globalfinding.com\"", 50001);
		udp_app_connect(RT_FALSE, connect_callback);
    //mqtt_app_init("iot.kebaidata.com",1883,"default_client","","");
    //mqtt_app_init("40.125.202.28",1883,"default_client","","");
    //mqtt_app_connect(RT_FALSE);

    //Lora startup
    /*
    service_manager_lora_init();
		lora_service_init();		
		lora_handler_init();
    */
    return 0;
}

void led_init(){
    /* start green SYS led */
    rt_pin_mode(LED_PIN_GREEN_SYS_2, PIN_MODE_OUTPUT);
    led_property_t* led_property_green = (led_property_t*)malloc(sizeof(led_property_t));
    strcpy(led_property_green->led_name, "green_sys");
    led_property_green->led_period_on   = 200;
    led_property_green->led_period_off  = 2000;
    led_property_green->led_turn_on     = turn_on_green_led;
    led_property_green->led_turn_off    = turn_off_green_led;
    manager_led_add(led_property_green);

    /* start blue Lora led */
    rt_pin_mode(LED_PIN_BLUE_LORA_1, PIN_MODE_OUTPUT);
    led_property_t* led_property_blue = (led_property_t*)malloc(sizeof(led_property_t));
    strcpy(led_property_blue->led_name, "blue_lora");
    led_property_blue->led_period_on    = 2000;
    led_property_blue->led_period_off   = 2000;
    led_property_blue->led_turn_on      = turn_on_blue_led;
    led_property_blue->led_turn_off     = turn_off_blue_led;
    manager_led_add(led_property_blue);
}
void connect_callback(lte_state_t lte_status){
    rt_kprintf("[LED] connect_callback called, status: %d\n", lte_status);
    
    if(lte_status == UDP_STATE_CONNECTED){ 
        rt_kprintf("[LED] Setting LED to fast blink (300, 300)\n");
        manager_led_reset_period("blue_lora",  300, 300); 
        manager_led_reset_period("green_sys",  300, 300); 
        service_t* service = query_service_by_type(SERVICE_TYPE_TIME_SYNC_REQUEST);
        if(service != RT_NULL) {
          rt_kprintf("Info: time sync request service executed\n");
          service->service_execute(RT_NULL, RT_NULL);   
        }     
    } 
    else{ 
        rt_kprintf("[LED] Setting LED to slow blink (2000, 2000)\n");
        manager_led_reset_period("blue_lora",  2000, 2000); 
        manager_led_reset_period("green_sys",  2000, 2000);
    } 
}