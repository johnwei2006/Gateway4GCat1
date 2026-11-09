#ifndef APPLICATIONS_SERVICE_MANAGER_H_
#define APPLICATIONS_SERVICE_MANAGER_H_

#include "../utils/cps_list.h"
#include "../service_manager/abstract_service.h"
#include "../transport_udp/transport_udp_message.h"
#include <stdbool.h>

extern bool is_service_udp_initialized;
extern struct cps_list_node *service_head_node;

extern rt_thread_t heartbeat_thread;
extern rt_thread_t period_report_thread;

int service_manager_udp_init(void);
void service_manager_finalize(void);
rt_err_t service_manager_process_downlink(transport_udp_message_t *transport_udp_message, void* business_data);
service_t* query_service_by_type(const uint8_t type);
void service_manager_period_execute(void *parameter);
void service_manager_heartbeat(void *parameter);
#endif /*APPLICATIONS_SERVICE_MANAGER_H_*/