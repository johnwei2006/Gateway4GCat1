#ifndef SERVICE_GATEWAY_DEVICE_CONTROL_H_
#define SERVICE_GATEWAY_DEVICE_CONTROL_H_

#include "abstract_service.h"
#include "../utils/utils.h"
#include "../config/config_manager.h"
#include "../transport_udp/transport_udp_message.h"
#include "../config/config_manager.h"
#include "../transport_udp/transport_udp_service_message.h"
#include "../modbus/modbus_manager.h"

typedef struct
{
    service_t           base;
    
} service_gateway_device_control_t;

rt_err_t service_execute_gateway_device_control(void* transport_udp_message, void* business_data);
service_gateway_device_control_t* service_gateway_device_control_create(void);
static bool device_control_is_success(rt_err_t return_value, modbus_request_message_t* modbus_request_message, modbus_response_message_t* modbus_response_message);

#endif /*SERVICE_GATEWAY_DEVICE_CONTROL_H_*/
