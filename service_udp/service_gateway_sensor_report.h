#ifndef _SERVICE_GATEWAY_SENSOR_REPORT_H_
#define _SERVICE_GATEWAY_SENSOR_REPORT_H_


#include "abstract_service.h"
#include "../utils/utils.h"
#include "../transport_udp/transport_udp_message.h"

typedef struct
{
    service_t                   base;
} service_gateway_sensor_report_t;    

rt_err_t service_execute_gateway_sensor_report(void* p_data, void* business_data);
service_gateway_sensor_report_t* service_gateway_sensor_report_create(void);

#endif /*_SERVICE_GATEWAY_SENSOR_REPORT_H_*/
