#ifndef APPLICATIONS_SERVICE_NODE_DEVICE_CONTROL_H_
#define APPLICATIONS_SERVICE_NODE_DEVICE_CONTROL_H_

#include "abstract_service.h"
#include "../utils/utils.h"
#include "../transport_udp/transport_udp_message.h"

typedef struct {
    uint8_t device_id;
    uint8_t action;
} node_device_control_t;

typedef struct {
    service_data_t              service_data;
    uint8_t                     node_id[4];
    uint8_t                     device_control_size;
    node_device_control_t       *device_control;
} node_device_control_data_t;

typedef struct
{
    service_t                    base;
    node_device_control_data_t   device_control_data;
} service_node_device_control_t;    

rt_err_t service_execute_node_device_control(void* p_data);
service_node_device_control_t* service_node_device_control_create(void);

#endif /*APPLICATIONS_SERVICE_NODE_DEVICE_CONTROL_H_*/
