#ifndef _SERVICE_NOTIFY_SENSOR_DATA_REPORT_BATCH_H_
#define _SERVICE_NOTIFY_SENSOR_DATA_REPORT_BATCH_H_


#include "abstract_service.h"
#include "../utils/utils.h"
#include "../transport_lora/lora_message.h"

typedef struct
{
    service_t                  base;
} service_notify_sensor_data_report_batch_t;    

rt_err_t service_execute_notify_sensor_data_report_batch(void* p_data);
service_notify_sensor_data_report_batch_t* service_notify_sensor_data_report_batch_create(void);  

#endif /*_SERVICE_NOTIFY_SENSOR_DATA_REPORT_BATCH_H_*/
