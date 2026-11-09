#ifndef APPLICATIONS_SERVICE_GATEWAY_TIME_SYNC_REQUEST_H_
#define APPLICATIONS_SERVICE_GATEWAY_TIME_SYNC_REQUEST_H_


#include "abstract_service.h"
#include "../utils/utils.h"
#include "../transport_udp/transport_udp_message.h"

typedef struct
{
    service_t                  base;
    service_data_t             base_data;
} service_gateway_time_sync_request_t;    

rt_err_t service_execute_gateway_time_sync_request(void* p_data, void* business_data);
service_gateway_time_sync_request_t* service_gateway_time_sync_request_create(void);  

#endif /*APPLICATIONS_SERVICE_GATEWAY_TIME_SYNC_REQUEST_H_*/