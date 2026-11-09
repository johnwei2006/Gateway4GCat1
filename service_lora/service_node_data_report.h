#ifndef _SERVICE_NODE_DATA_REPORT_H_
#define _SERVICE_NODE_DATA_REPORT_H_


#include "abstract_service.h"
#include "../utils/utils.h"
#include "../transport_udp/transport_udp_message.h"

typedef struct
{
    service_t                   base;
} service_node_data_report_t;    

rt_err_t service_execute_node_data_report(void* p_data);
service_node_data_report_t* service_node_data_report_create(void);

#endif /*_SERVICE_NODE_DATA_REPORT_H_*/
