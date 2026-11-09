#ifndef APPLICATIONS_SERVICE_MANAGER_LORA_H_
#define APPLICATIONS_SERVICE_MANAGER_LORA_H_

#include "../utils/cps_list.h"
#include "../service_manager/abstract_service.h"
#include "../transport_lora/lora_message.h"
#include <stdbool.h>

static bool is_service_lora_initialized = false;
static struct cps_list_node *service_head_node;

static rt_thread_t period_execute_service_lora_thread = RT_NULL;

int service_manager_lora_init(void);
void service_manager_lora_finalize(void);
rt_err_t service_manager_lora_process_uplink(PHYPayload_t *PHYPayload);
static service_t* query_lora_service_by_type(const uint8_t type);
#endif /*APPLICATIONS_SERVICE_MANAGER_LORA_H_*/
