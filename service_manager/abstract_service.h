#ifndef APPLICATIONS_ABSTRACT_SERVICE_H_
#define APPLICATIONS_ABSTRACT_SERVICE_H_

#include <rtthread.h>

#pragma pack(1)

typedef enum {
    service_status_running    = 1,
    service_status_stop       = 0,
} service_status_t;

typedef struct
{
    uint8_t             service_type;
    service_status_t    service_status;
    rt_err_t            (*service_execute)(void* p_data, void* business_data);
} service_t;

#endif /*APPLICATIONS_ABSTRACT_SERVICE_H_*/
