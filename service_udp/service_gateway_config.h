#ifndef SERVICE_GATEWAY_CONFIG_H_
#define SERVICE_GATEWAY_CONFIG_H_

#include "abstract_service.h"
#include "../utils/utils.h"
#include "../config/config_manager.h"
#include "../transport_udp/transport_udp_message.h"
#include "../transport_udp/transport_udp_service_message.h"
#include "../config/config_manager.h"

typedef struct
{
    service_t           base;
} service_gateway_config_t;

rt_err_t service_execute_gateway_config(void* transport_udp_message, void* business_data);
service_gateway_config_t* service_gateway_config_create(void);

#endif /*SERVICE_GATEWAY_CONFIG_H_*/
