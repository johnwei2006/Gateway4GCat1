#ifndef LORA_HANDLER_H_
#define LORA_HANDLER_H_

#include <rtthread.h>
#include "../transport_lora/lora_message.h"

int lora_handler_init(void);
static rt_thread_t lora_uplink_thread = RT_NULL;
static rt_thread_t lora_downlink_thread = RT_NULL;
static void lora_uplink_process(void *parameter);
static void lora_downlink_process(void *parameter);
static void lora_message_mem_init(lora_message_t *lora_tx_message, PHYPayload_t *PHYPayload);
extern void lora_trigger_tx(void);
extern void lora_trigger_lora_uplink_sem(void);

extern rt_sem_t lora_uplink_sem;
extern rt_sem_t lora_downlink_sem;
#endif /*LORA_HANDLER_H_*/

