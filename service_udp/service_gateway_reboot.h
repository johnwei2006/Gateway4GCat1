#ifndef _SERVICE_GATEWAY_REBOOT_H_
#define _SERVICE_GATEWAY_REBOOT_H_


#include "abstract_service.h"
#include "../utils/utils.h"
#include "../transport_udp/transport_udp_message.h"

typedef struct
{
    service_t                   base;
} service_gateway_reboot_t;    

rt_err_t service_execute_gateway_reboot(void* transport_udp_message, void* business_data);
service_gateway_reboot_t* service_gateway_reboot_create(void);

#endif /*_SERVICE_GATEWAY_REBOOT_H_*/
