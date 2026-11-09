#include "service_gateway_config.h"
#include "../gateway_manager/rtc_manager.h"

service_gateway_config_t* service_gateway_config_create(){
    service_gateway_config_t* service_gateway_config = (service_gateway_config_t*)rt_malloc(sizeof(service_gateway_config_t));
    service_gateway_config->base.service_type 		= SERVICE_TYPE_GATEWAY_CONFIG;
    service_gateway_config->base.service_execute 	= service_execute_gateway_config;
    return service_gateway_config;
}

rt_err_t service_execute_gateway_config(void* transport_udp_message, void* business_data){
    if(transport_udp_message == RT_NULL)
        return RT_EINVAL;
    transport_udp_message_t *transport_udp_message_ptr = (transport_udp_message_t*)transport_udp_message;
     config_gateway_serial_t *gateway_config = RT_NULL;
    get_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id, (void**)&gateway_config);
    if(gateway_config == RT_NULL)       
        return RT_EINVAL;

    rt_kprintf("Info: Updating gateway configuration...\n");
    rt_kprintf("Info: Gateway ID: %s\n", gateway_config->gateway_id);
    rt_kprintf("Info: project_id: %s\n", gateway_config->project_id);
    rt_kprintf("Info: system_server_url: %s\n", gateway_config->system_server_url);
    rt_kprintf("Info: system_server_port: %d\n", gateway_config->system_server_port);
    //rt_kprintf("Info: business_server_url: %s\n", gateway_config->business_server_url);
    rt_kprintf("Info: heartbeat_interval: %d\n", gateway_config->heartbeat_interval);
    rt_kprintf("Info: report_interval: %d\n", gateway_config->report_interval);
    rt_kprintf("Info: lora_freq_group: %d\n", gateway_config->config_lora.loraFreqGroup);

    service_data_config_response_t* response = (service_data_config_response_t*)rt_malloc(sizeof(service_data_config_response_t));
    if(gateway_config == RT_NULL || gateway_config->gateway_id == RT_NULL){
        rt_kprintf("Error: gateway_config is NULL\n");
        response->result = RT_EEMPTY;
        strcpy(response->message, "gateway_config is NULL");
        goto exit;
    } else {
        config_gateway_serial_t gateway_config_existed = {0};
        flash_status status = read_config_gateway_serial(&gateway_config_existed);
        if(status != FLASH_OK){
            rt_kprintf("Error: read_config_gateway_serial failed\n");
            response->result = RT_EEMPTY;
            strcpy(response->message, "no exist gateway config");
            goto exit;
        }
        if(strcmp(gateway_config_existed.gateway_id, gateway_config->gateway_id) != 0){
            rt_kprintf("Error: gateway_id not match\n");
            response->result = RT_EEMPTY;
            strcpy(response->message, "gateway_id not match");
            goto exit;
        }
        rt_device_rtc_time_t rtc_time = {0};
        if (read_rtc_time(&rtc_time) == 0) {
            gateway_config->config_time[0] = rtc_time.year % 100;
            gateway_config->config_time[1] = rtc_time.month;
            gateway_config->config_time[2] = rtc_time.day;
            gateway_config->config_time[3] = rtc_time.hour;
            gateway_config->config_time[4] = rtc_time.min;
            gateway_config->config_time[5] = rtc_time.sec;
        } else {
            rt_kprintf("Error: Failed to read RTC time\n");
        }
        status = update_config_gateway(gateway_config);
        if(status == FLASH_OK){
            response->result = RT_EOK;
            strcpy(response->message, "update gateway config success");
        } else {
            response->result = status;
            strcpy(response->message, "update gateway config failed");
        }
			}
exit:
    remove_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id);
	transport_udp_message_t* gateway_uplink_message  	= (transport_udp_message_t*)rt_malloc(sizeof(transport_udp_message_t));
	if(gateway_uplink_message == NULL)
	{
		rt_kprintf("memory allocate gateway_uplink_message failed. \n");
        return RT_ENOMEM;
	}

	rt_memset(gateway_uplink_message, 0, sizeof(transport_udp_message_t));
		
	gateway_uplink_message->transport_udp_header			= (transport_udp_header_t*)rt_malloc(sizeof(transport_udp_header_t));
	gateway_uplink_message->transport_udp_data				= (transport_udp_data_t*)rt_malloc(sizeof(transport_udp_data_t));
	gateway_uplink_message->service_data					= (service_data_t*)rt_malloc(sizeof(service_data_t));
		
  rt_memset(gateway_uplink_message->transport_udp_header, 0, sizeof(transport_udp_header_t));
  rt_memset(gateway_uplink_message->transport_udp_data, 	0, sizeof(transport_udp_data_t));
  rt_memset(gateway_uplink_message->service_data, 		0, sizeof(service_data_t));
        
	gateway_uplink_message->transport_udp_data->event_id    	= transport_udp_message_ptr->transport_udp_data->event_id; //generate_rand_short();
	gateway_uplink_message->transport_udp_data->service_type  	= SERVICE_TYPE_GATEWAY_CONFIG_RESPONSE;
  gateway_uplink_message->transport_udp_data->service_version = SERVICE_VERSION_1; 
  gateway_uplink_message->service_data->business_data_payload = RT_NULL;
  gateway_uplink_message->service_data->business_data_payload_length= 0;
  gateway_uplink_message->service_data->business_data 		= RT_NULL;

  put_business_data_cache(gateway_uplink_message->transport_udp_data->event_id, response);
        rt_err_t result = put_gateway_uplink_message(&gateway_uplink_message);
        if(result != RT_EOK){
            rt_kprintf("Warning: put_gateway_uplink_message failed, free memory\n");
            free_transport_udp_message(gateway_uplink_message);  // 正确释放内存
            return result;
        }        
		return RT_EOK;
};