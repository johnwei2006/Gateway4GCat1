#ifndef APPLICATIONS_SERVICE_GATEWAY_INFO_QUERY_H_
#define APPLICATIONS_SERVICE_GATEWAY_INFO_QUERY_H_


#include "abstract_service.h"
#include "../utils/utils.h"
#include "../transport_udp/transport_udp_message.h"
#include "../transport_udp/transport_udp_service_message.h"
#include "../gateway_manager/gateway_info.h"

typedef struct
{
    service_t                       base;
    gateway_info_t                  gateway_info;
} service_gateway_info_query_t;    

rt_err_t service_execute_gateway_info_query(void* transport_udp_message, void* business_data);
service_gateway_info_query_t* service_gateway_info_query_create(void);

#endif /*APPLICATIONS_SERVICE_GATEWAY_INFO_QUERY_H_*/
