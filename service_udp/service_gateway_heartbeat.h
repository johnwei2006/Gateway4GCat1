#ifndef APPLICATIONS_SERVICE_GATEWAY_HEARTBEAT_H_
#define APPLICATIONS_SERVICE_GATEWAY_HEARTBEAT_H_


#include "abstract_service.h"
#include "../utils/utils.h"
#include "../transport_udp/transport_udp_message.h"

typedef struct
{
    service_t                   base;
} service_gateway_heartbeat_t;    

rt_err_t service_execute_gateway_heartbeat(void* p_data, void* business_data);
service_gateway_heartbeat_t* service_gateway_heartbeat_create(void);

#endif /*APPLICATIONS_SERVICE_GATEWAY_HEARTBEAT_H_*/