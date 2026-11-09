#ifndef APPLICATIONS_SERVICE_NODE_JOIN_REQUEST_H_
#define APPLICATIONS_SERVICE_NODE_JOIN_REQUEST_H_

#include "abstract_service.h"
#include "../utils/utils.h"
#include "../config/config_manager.h"

typedef struct {
    uint8_t             node_id[NODE_ID_LENGTH];
    uint8_t             firm_type;
    uint8_t             firm_version[4];
    uint8_t             uboot_version[4];
    uint8_t             mcu_id[10];
    uint8_t             freq_group;
} node_join_request_data_t;

typedef struct
{
    service_t                   base;
    node_join_request_data_t    join_request_data;
} service_node_join_request_t;    

void service_execute_node_join_request(void* p_data);
service_node_join_request_t* service_node_join_request_create();

#endif /*APPLICATIONS_SERVICE_NODE_JOIN_REQUEST_H_*/
