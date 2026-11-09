#ifndef _SERVICE_PING_H_
#define _SERVICE_PING_H_


#include "abstract_service.h"
#include "../utils/utils.h"
#include "../transport_lora/lora_message.h"

typedef struct
{
    service_t                  base;
} service_ping_t;    

rt_err_t service_execute_ping(void* p_data);
service_ping_t* service_ping_create(void);  

#endif /*_SERVICE_PING_H_*/
