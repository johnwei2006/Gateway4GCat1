#include "transport_udp_encoder.h"
#include "../transport_udp/transport_udp_message.h"
#include "../transport_udp/transport_udp_service_message.h"
#include "../transport_lora/lora_message.h"
#include "../config/config_manager.h"
#include "../gateway_manager/gateway_info.h"
#include "../transport_udp/transport_message_cache.h"
#include "../transport_udp/business_data_cache.h"

void gateway_manager_get_reboot_time(uint8_t* reboot_time);
// from udp_message->service_data->business_data_payload to udp_message->service_data->business_data
int transport_udp_service_decode(transport_udp_message_t *udp_message, void  **business_data){
    uint8_t service_type = udp_message->transport_udp_data->service_type;
    int ret = 0;
    switch(service_type){
        case SERVICE_TYPE_HEARTBEAT:
            udp_message->service_data->business_data                = RT_NULL;
            udp_message->service_data->business_data_payload        = RT_NULL;
            udp_message->service_data->business_data_payload_length = 0;
            break;
        case SERVICE_TYPE_TIME_SYNC_RESPONSE:	
             if(udp_message->service_data->business_data_payload_length < sizeof(service_data_time_sync_response_t)){
                rt_kprintf("Error: Invalid business data payload length for time sync response\n");
                ret = -RT_EINVAL;
                break;
            }          
            service_data_time_sync_response_t* time_sync_response = (service_data_time_sync_response_t *)rt_malloc(sizeof(service_data_time_sync_response_t));
			      time_sync_response->year   = udp_message->service_data->business_data_payload[0];
            time_sync_response->month  = udp_message->service_data->business_data_payload[1];
            time_sync_response->day    = udp_message->service_data->business_data_payload[2];
            time_sync_response->hour   = udp_message->service_data->business_data_payload[3];
            time_sync_response->minute = udp_message->service_data->business_data_payload[4];
            time_sync_response->second = udp_message->service_data->business_data_payload[5];
            *business_data = time_sync_response;
            //put_business_data_cache(udp_message->transport_udp_data->event_id, time_sync_response);
            break;
        case SERVICE_TYPE_GATEWAY_DEVICE_CONTROL:
            if(udp_message->service_data->business_data_payload_length < sizeof(service_data_gateway_device_control_t)){
                rt_kprintf("Error: Invalid business data payload length for gateway device control\n");
                ret = -RT_EINVAL;
                break;
            }           
            //decode device control message
            //1. allocate memory for device_control 
            service_data_gateway_device_control_t* device_control = rt_malloc(sizeof(service_data_gateway_device_control_t));
            if (device_control == NULL) {
                rt_kprintf("malloc device_control 失败\n");
                ret = -RT_ENOMEM;
                break;
            }
            rt_memset(device_control, 0, sizeof(service_data_gateway_device_control_t));

            //2. parse business_data_payload to device_control
            rt_memcpy(device_control->node_id, &udp_message->service_data->business_data_payload[0], NODE_ID_LENGTH);
            device_control->gateway_device_control_items_length = (udp_message->service_data->business_data_payload_length - NODE_ID_LENGTH - 1) / DEVICE_CONTROL_ITEM_LENGTH;

            if(udp_message->service_data->business_data_payload_length < sizeof(service_data_gateway_device_control_t) + sizeof(service_data_gateway_device_control_item_t) * device_control->gateway_device_control_items_length){
                rt_kprintf("Error: Invalid business data payload length for gateway device control\n");
                ret = -RT_EINVAL;
                break;
            }           
            //3. allocate memory for gateway_device_control_items
            device_control->gateway_device_control_items = rt_malloc(sizeof(service_data_gateway_device_control_item_t) * device_control->gateway_device_control_items_length);
            if (device_control->gateway_device_control_items == NULL) {
                rt_kprintf("malloc gateway_device_control_items 失败\n");
                rt_free(device_control);
                ret = -RT_ENOMEM;
                break;
            }
            rt_memset(device_control->gateway_device_control_items, 0, sizeof(service_data_gateway_device_control_item_t) * device_control->gateway_device_control_items_length);
            //4. parse business_data_payload to gateway_device_control_items
            for(uint8_t i = 0; i < device_control->gateway_device_control_items_length; i++){
                device_control->gateway_device_control_items[i].channel = udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*4];
                device_control->gateway_device_control_items[i].action  = udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 2 + i*4];
                rt_memcpy(device_control->gateway_device_control_items[i].value, &udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 3 + i*4], 4);
            }
            udp_message->service_data->business_data = device_control;
            put_business_data_cache(udp_message->transport_udp_data->event_id, device_control);//, sizeof(service_data_gateway_device_control_t) + sizeof(service_data_gateway_device_control_item_t) * device_control->gateway_device_control_items_length);
            break;
        case SERVICE_TYPE_GATEWAY_CONFIG:
            if(udp_message->service_data->business_data_payload_length < GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 8){
                rt_kprintf("Error: Invalid business data payload length for gateway config\n");
                ret = -RT_EINVAL;
                break;
            }           
            //decode gateway config message
            config_gateway_serial_t* gateway_config = (config_gateway_serial_t *)rt_malloc(sizeof(config_gateway_serial_t));
            gateway_config->config_version = udp_message->service_data->business_data_payload[0];
            rt_memcpy(gateway_config->gateway_id, udp_message->service_data->business_data_payload + 1, GATEWAY_ID_LENGTH);
            rt_memcpy(gateway_config->project_id, udp_message->service_data->business_data_payload + GATEWAY_ID_LENGTH + 1, PROJECT_ID_MAX_LENGTH);
            rt_memcpy(gateway_config->system_server_url, udp_message->service_data->business_data_payload + (GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + 1), SERVER_URL_MAX_LENGTH);
            gateway_config->system_server_port = (uint16_t)(udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 1] << 8 | udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 2]);
            //rt_memcpy(gateway_config->business_server_url, &udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH], SERVER_URL_MAX_LENGTH);
            gateway_config->heartbeat_interval        = (uint16_t)(udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 3] << 8 | udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 4]);    
            gateway_config->report_interval           = (uint16_t)(udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 5] << 8 | udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 6]);    
            gateway_config->config_lora.loraFreqGroup = udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 7];
            put_business_data_cache(udp_message->transport_udp_data->event_id, gateway_config);
            break;
        case SERVICE_TYPE_GATEWAY_CONFIG_NODE:
            if(udp_message->service_data->business_data_payload_length < NODE_ID_LENGTH + 4){
                rt_kprintf("Error: Invalid business data payload length for node config\n");
                ret = -RT_EINVAL;
                break;
            }            
            //decode gateway node config message
            service_data_config_node_t *node_config = (service_data_config_node_t *)rt_malloc(sizeof(service_data_config_node_t));
            node_config->node_size = udp_message->service_data->business_data_payload[0];
        
            node_config->node_config_items = rt_malloc(sizeof(service_data_config_node_item_t) * node_config->node_size);
            for(uint8_t i = 0; i < node_config->node_size; i++){
                node_config->node_config_items[i].node_type = udp_message->service_data->business_data_payload[i*(NODE_ID_LENGTH + 4) + 1];
                rt_memcpy(node_config->node_config_items[i].node_id, &udp_message->service_data->business_data_payload[i*(NODE_ID_LENGTH + 4) + 2], NODE_ID_LENGTH);
                node_config->node_config_items[i].report_interval = (uint16_t)udp_message->service_data->business_data_payload[i*(NODE_ID_LENGTH + 4) + 2 + NODE_ID_LENGTH] << 8 | udp_message->service_data->business_data_payload[i*(NODE_ID_LENGTH + 4) + 3 + NODE_ID_LENGTH];
                node_config->node_config_items[i].lora_freq_group = udp_message->service_data->business_data_payload[i*(NODE_ID_LENGTH + 4) + 4 + NODE_ID_LENGTH];
            }
            put_business_data_cache(udp_message->transport_udp_data->event_id, node_config);
            break;
        case SERVICE_TYPE_GATEWAY_CONFIG_SENSOR:
            uint8_t min_payload_length = NODE_ID_LENGTH + 1 + (3 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH);
            if(udp_message->service_data->business_data_payload_length < min_payload_length){
                rt_kprintf("Error: Invalid business data payload length for sensor config\n");
                ret = -RT_EINVAL;
                break;
            }        
            //decode gateway sensor report config message
            config_sensor_list_t *sensor_config = (config_sensor_list_t *)rt_calloc(1, sizeof(config_sensor_list_t));
            rt_memcpy(sensor_config->node_id, udp_message->service_data->business_data_payload, NODE_ID_LENGTH);
            sensor_config->sensor_count = udp_message->service_data->business_data_payload[NODE_ID_LENGTH];

            for(uint8_t i = 0; i < sensor_config->sensor_count; i++){
                sensor_config->sensors[i].channel = udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(3 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH)];
                sensor_config->sensors[i].sensor_type = (uint16_t)udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(3 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH) + 1] << 8 | udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(3 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH) + 2];
                rt_memcpy(sensor_config->sensors[i].sensor_code, &udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(3 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH) + 3], DEVICE_CODE_MAX_LENGTH);
                rt_memcpy(sensor_config->sensors[i].read_hex, &udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(3 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH) + 3 + DEVICE_CODE_MAX_LENGTH], COMMAND_HEX_MAX_LENGTH);
            }
            put_business_data_cache(udp_message->transport_udp_data->event_id, sensor_config);
            break;  
        case SERVICE_TYPE_GATEWAY_CONFIG_DEVICE:
            min_payload_length = NODE_ID_LENGTH + 1 + (4 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH*3);
            if(udp_message->service_data->business_data_payload_length < min_payload_length){
                rt_kprintf("Error: Invalid business data payload length for device config\n");
                ret = -RT_EINVAL;
                break;
            }        
            //decode gateway device config message
            config_device_list_t *device_config = (config_device_list_t *)rt_calloc(1,sizeof(config_device_list_t));
            rt_memcpy(device_config->node_id, udp_message->service_data->business_data_payload, NODE_ID_LENGTH);
            device_config->device_count = udp_message->service_data->business_data_payload[NODE_ID_LENGTH];

            for(uint8_t i = 0; i < device_config->device_count; i++){
                device_config->devices[i].channel = udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(4 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH*3)];
                device_config->devices[i].switch_type = udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(4 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH*3) + 1];
                device_config->devices[i].device_type = (uint16_t)udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(4 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH*3) + 2] << 8 | udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(4 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH*3) + 3];
                rt_memcpy(device_config->devices[i].device_code, &udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(4 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH*3) + 4], DEVICE_CODE_MAX_LENGTH);
                rt_memcpy(device_config->devices[i].on_hex, &udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(4 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH*3) + 4 + DEVICE_CODE_MAX_LENGTH], COMMAND_HEX_MAX_LENGTH);
                rt_memcpy(device_config->devices[i].off_hex, &udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(4 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH*3) + 4 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH], COMMAND_HEX_MAX_LENGTH);
                rt_memcpy(device_config->devices[i].set_hex, &udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(4 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH*3) + 4 + DEVICE_CODE_MAX_LENGTH + 2*COMMAND_HEX_MAX_LENGTH], COMMAND_HEX_MAX_LENGTH);
            }
            put_business_data_cache(udp_message->transport_udp_data->event_id, device_config);
            break;  
        case SERVICE_TYPE_GATEWAY_CONFIG_SENSOR_QUERY:
        case SERVICE_TYPE_GATEWAY_CONFIG_DEVICE_QUERY:
        case SERVICE_TYPE_GATEWAY_SENSOR_DATA_UPLOAD_NOTICE:
            if(udp_message->service_data->business_data_payload_length < NODE_ID_LENGTH){
                rt_kprintf("Error: Invalid business data payload length for query or notice\n");
                ret = -RT_EINVAL;
                break;
            }
            uint8_t *node_id = (uint8_t*)rt_malloc(NODE_ID_LENGTH);
            rt_memcpy(node_id, udp_message->service_data->business_data_payload, NODE_ID_LENGTH); 
            udp_message->service_data->business_data = node_id;
            break;
        case SERVICE_TYPE_GATEWAY_REBOOT:
            if(udp_message->service_data->business_data_payload_length < 2){
                rt_kprintf("Error: Invalid business data payload length for reboot\n");
                ret = -RT_EINVAL;
                break;
            }        
            //decode gateway reboot message
            service_data_reboot_t *service_data_reboot = (service_data_reboot_t *)rt_malloc(sizeof(service_data_reboot_t));
            service_data_reboot->app_type = udp_message->service_data->business_data_payload[0];
            service_data_reboot->action = udp_message->service_data->business_data_payload[1];
            put_business_data_cache(udp_message->transport_udp_data->event_id, service_data_reboot);
            break;
        case SERVICE_TYPE_GATEWAY_UPGRADE:
            if(udp_message->service_data->business_data_payload_length < 2){
                rt_kprintf("Error: Invalid business data payload length for upgrade\n");
                ret = -RT_EINVAL;
                break;
            }        
            //decode gateway upgrade message
            service_data_gateway_upgrade_t *service_data_upgrade = (service_data_gateway_upgrade_t *)rt_malloc(sizeof(service_data_gateway_upgrade_t));

            rt_memcpy(service_data_upgrade->upgrade_id, udp_message->service_data->business_data_payload, 64);
            service_data_upgrade->app_type = udp_message->service_data->business_data_payload[64];
            service_data_upgrade->upgrade_type = udp_message->service_data->business_data_payload[65];
            rt_memcpy(service_data_upgrade->version, udp_message->service_data->business_data_payload + 65, 32);
            rt_memcpy(service_data_upgrade->download_url, udp_message->service_data->business_data_payload + 97, 256);
            service_data_upgrade->firmware_size = (uint32_t)udp_message->service_data->business_data_payload[356] << 24 | udp_message->service_data->business_data_payload[355] << 16 | udp_message->service_data->business_data_payload[354] << 8 | udp_message->service_data->business_data_payload[353];
            rt_memcpy(service_data_upgrade->md5, udp_message->service_data->business_data_payload + 357, 33);
            put_business_data_cache(udp_message->transport_udp_data->event_id, service_data_upgrade);
            break;
        case SERVICE_TYPE_GATEWAY_CONFIG_TASK:
            if(udp_message->service_data->business_data_payload_length < 43){
                rt_kprintf("Error: Invalid business data payload length for task\n");
                ret = -RT_EINVAL;
                break;
            }
            
            //decode gateway task message
            config_gateway_task_t *config_task = (config_gateway_task_t *)rt_malloc(sizeof(config_gateway_task_t));
            config_task->task_id = udp_message->service_data->business_data_payload[0];
            config_task->enabled = udp_message->service_data->business_data_payload[2];
            rt_memcpy(config_task->trigger_time, &udp_message->service_data->business_data_payload[8], 4);
            config_task->trigger_size = udp_message->service_data->business_data_payload[12];
            for(int i = 0; i < config_task->trigger_size; i++){
                config_task->triggers[i].join_type = udp_message->service_data->business_data_payload[13 + i];
                rt_memcpy(config_task->triggers[i].node_id, &udp_message->service_data->business_data_payload[14 + i], NODE_ID_LENGTH);
                config_task->triggers[i].channel = udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH];
                config_task->triggers[i].compare_type = udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH + 1];
                config_task->triggers[i].value = (uint32_t)udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH + 2] << 24 | udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH + 3] << 16 | udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH + 4] << 8 | udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH + 5];  
            }
            config_task->action_size = udp_message->service_data->business_data_payload[12 + config_task->trigger_size * (NODE_ID_LENGTH + 7)];
            for(int i = 0; i < config_task->action_size; i++){
                rt_memcpy(config_task->actions[i].node_id, &udp_message->service_data->business_data_payload[14 + i], NODE_ID_LENGTH);
                config_task->actions[i].channel = udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH];
                config_task->actions[i].action = udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH + 1];
                config_task->actions[i].value = (uint16_t)udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH + 4] << 8 | udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH + 5];
            }

            put_business_data_cache(udp_message->transport_udp_data->event_id, config_task);
            break;
        case SERVICE_TYPE_GATEWAY_CONFIG_TASK_LIST:
           if(udp_message->service_data->business_data_payload_length < 43){
                rt_kprintf("Error: Invalid business data payload length for task list\n");
                ret = -RT_EINVAL;
                break;
            }          
            //decode gateway task message
            config_gateway_task_list_t *config_task_list = (config_gateway_task_list_t *)rt_malloc(sizeof(config_gateway_task_list_t));
            config_task_list->task_count = udp_message->service_data->business_data_payload[0];
            for(int i = 0; i < config_task_list->task_count; i++){
                config_task_list->config_tasks[i].task_id = udp_message->service_data->business_data_payload[1 + i];
                config_task_list->config_tasks[i].enabled = udp_message->service_data->business_data_payload[2 + i];
                config_task_list->config_tasks[i].trigger_type = udp_message->service_data->business_data_payload[3 + i];
                rt_memcpy(&config_task_list->config_tasks[i].trigger_time, &udp_message->service_data->business_data_payload[4 + i], 3);
                config_task_list->config_tasks[i].trigger_size = udp_message->service_data->business_data_payload[8 + i];
                for(int j = 0; j < config_task_list->config_tasks[i].trigger_size; j++){
                    config_task_list->config_tasks[i].triggers[j].join_type = udp_message->service_data->business_data_payload[5 + i];
                    rt_memcpy(&config_task_list->config_tasks[i].triggers[j].node_id, &udp_message->service_data->business_data_payload[5 + i], NODE_ID_LENGTH);
                    config_task_list->config_tasks[i].triggers[j].channel = udp_message->service_data->business_data_payload[5 + i + NODE_ID_LENGTH];
                    config_task_list->config_tasks[i].triggers[j].compare_type = udp_message->service_data->business_data_payload[5 + i + NODE_ID_LENGTH + 1];
                    config_task_list->config_tasks[i].triggers[j].value = (uint32_t)udp_message->service_data->business_data_payload[5 + i + NODE_ID_LENGTH + 2] << 24 | udp_message->service_data->business_data_payload[5 + i + NODE_ID_LENGTH + 3] << 16 | udp_message->service_data->business_data_payload[5 + i + NODE_ID_LENGTH + 4] << 8 | udp_message->service_data->business_data_payload[5 + i + NODE_ID_LENGTH + 5];  
                }
                config_task_list->config_tasks[i].action_size = udp_message->service_data->business_data_payload[12 + i];
                for(int j = 0; j < config_task_list->config_tasks[i].action_size; j++){
                    rt_memcpy(&config_task_list->config_tasks[i].actions[j].node_id, &udp_message->service_data->business_data_payload[14 + i], NODE_ID_LENGTH);
                    config_task_list->config_tasks[i].actions[j].channel = udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH];
                    config_task_list->config_tasks[i].actions[j].action = udp_message->service_data->business_data_payload[14 + i + NODE_ID_LENGTH + 1];
                    config_task_list->config_tasks[i].actions[j].value = (uint16_t)udp_message->service_data->business_data_payload[5 + i + NODE_ID_LENGTH + 4] << 8 | udp_message->service_data->business_data_payload[5 + i + NODE_ID_LENGTH + 5];
                }
            }
            put_business_data_cache(udp_message->transport_udp_data->event_id, config_task_list);
            break;
        case SERVICE_TYPE_GATEWAY_INFO_QUERY:
        case SERVICE_TYPE_GATEWAY_CONFIG_QUERY:
        case SERVICE_TYPE_GATEWAY_CONFIG_NODE_QUERY:
        case SERVICE_TYPE_GATEWAY_CONFIG_TASK_QUERY:
        default:
             udp_message->service_data->business_data = RT_NULL;
    }
	return ret;
}
// from udp_message->service_data->business_data to udp_message->service_data->business_data_payload
int transport_udp_service_encode(transport_udp_message_t *udp_message){
    int ret = 0;
    if (udp_message == RT_NULL || udp_message->transport_udp_data == RT_NULL || udp_message->service_data == RT_NULL) {
        rt_kprintf("Error: transport_udp_service_encode invalid parameters\n");
        return -RT_EINVAL;
    }
    unsigned char service_type = udp_message->transport_udp_data->service_type;

    switch(service_type){
        case SERVICE_TYPE_GATEWAY_INFO_REPORT:
            gateway_info_t* gateway_info = RT_NULL;
            get_business_data_cache(udp_message->transport_udp_data->event_id, (void**)&gateway_info);        
            if(gateway_info == RT_NULL){
                rt_kprintf("Error: Failed to get gateway_info from cache\n");
                ret = -RT_ENOMEM;
                break;
            }        

            udp_message->service_data->business_data_payload_length = GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 36;

            udp_message->service_data->business_data_payload = (uint8_t*)rt_malloc(udp_message->service_data->business_data_payload_length);
            if(udp_message->service_data->business_data_payload == RT_NULL){
                rt_kprintf("Error: Failed to allocate memory for gateway report payload, size: %d\n", udp_message->service_data->business_data_payload_length);
                rt_free(gateway_info->lte_info);
                rt_free(gateway_info->gateway_config);
                ret = -RT_ENOMEM;
                break;
            }

            udp_message->service_data->business_data_payload[0] = gateway_info->gateway_config->config_version;
            rt_memcpy(&udp_message->service_data->business_data_payload[1], gateway_info->gateway_config->gateway_id, GATEWAY_ID_LENGTH);
            rt_memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1], gateway_info->gateway_config->project_id, PROJECT_ID_MAX_LENGTH);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH] = (uint8_t)(gateway_info->gateway_config->heartbeat_interval & 0xFF);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 1] = (uint8_t)(gateway_info->gateway_config->heartbeat_interval >> 8);

            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 2] = (uint8_t)(gateway_info->gateway_config->report_interval & 0xFF);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 3] = (uint8_t)(gateway_info->gateway_config->report_interval >> 8);

            rt_memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4],     gateway_info->rtc_time, 6);
            rt_memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6], gateway_info->gateway_config->system_server_url, SERVER_URL_MAX_LENGTH);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH]     = (uint8_t)(gateway_info->gateway_config->system_server_port & 0xFF);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 1] = (uint8_t)(gateway_info->gateway_config->system_server_port >> 8);   
            //rt_memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH], gateway_info->gateway_config->business_server_url, SERVER_URL_MAX_LENGTH);

            rt_memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2],gateway_info->hardware_version, FIRMWARE_VERSION_LENGTH);
            rt_memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + FIRMWARE_VERSION_LENGTH],gateway_info->firmware_version, FIRMWARE_VERSION_LENGTH);

            uint8_t mcu_id_str[12] = {0};
            snprintf(mcu_id_str, sizeof(mcu_id_str), "%08X%08X%08X",
                    gateway_info->gateway_mcuid[0],
                    gateway_info->gateway_mcuid[1],
                    gateway_info->gateway_mcuid[2]);
            rt_memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH], mcu_id_str, 12);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 12] = (uint8_t)(gateway_info->memory_heap_total & 0xFF);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 13] = (uint8_t)(gateway_info->memory_heap_total >> 8);
            
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 14] = (uint8_t)(gateway_info->memory_heap_used & 0xFF);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 15] = (uint8_t)(gateway_info->memory_heap_used >> 8);
            
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 16] = (uint8_t)(gateway_info->flash_total & 0xFF);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 17] = (uint8_t)(gateway_info->flash_total >> 8);
            
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 18] = (uint8_t)(gateway_info->flash_used & 0xFF);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 19]= (uint8_t)(gateway_info->flash_used >> 8);

            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 20] = (uint8_t)(gateway_info->uplink_message_size & 0xFF);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 21]= (uint8_t)(gateway_info->uplink_message_size >> 8);

            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 22] = (uint8_t)(gateway_info->downlink_message_size & 0xFF);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 23]= (uint8_t)(gateway_info->downlink_message_size >> 8);

            memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 24], &gateway_info->battery_voltage, 4);

            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 28] = gateway_info->adapter_exists;
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 29] = gateway_info->charging_enabled;           
            gateway_manager_get_reboot_time(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1 + PROJECT_ID_MAX_LENGTH + 4 + 6 + SERVER_URL_MAX_LENGTH + 2 + 2*FIRMWARE_VERSION_LENGTH + 30]);
            rt_free(gateway_info->lte_info);
            rt_free(gateway_info->gateway_config);
            break;
        case SERVICE_TYPE_GATEWAY_CONFIG_REPORT:
            config_gateway_serial_t* gateway_config = RT_NULL;
            get_business_data_cache(udp_message->transport_udp_data->event_id, (void**)&gateway_config);        
            if(gateway_config == RT_NULL){
                rt_kprintf("Error: Failed to get gateway_config from cache\n");
                ret = -RT_ENOMEM;
                break;
            } 
            
            udp_message->service_data->business_data_payload_length = GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 8 + 6;
            udp_message->service_data->business_data_payload = (uint8_t*)rt_malloc(udp_message->service_data->business_data_payload_length);
            if(udp_message->service_data->business_data_payload == RT_NULL){
                rt_kprintf("Error: Failed to malloc memory for business data payload\n");
                rt_free(gateway_config);
                ret = -RT_ENOMEM;
                break;
            }
            udp_message->service_data->business_data_payload[0] = gateway_config->config_version;
            rt_memcpy(&udp_message->service_data->business_data_payload[1], gateway_config->gateway_id, GATEWAY_ID_LENGTH);
            rt_memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + 1], gateway_config->project_id, PROJECT_ID_MAX_LENGTH);
            //rt_memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + 1], gateway_config->business_server_url, SERVER_URL_MAX_LENGTH);
            rt_memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + 1], gateway_config->system_server_url, SERVER_URL_MAX_LENGTH);
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 1] = gateway_config->system_server_port >> 8;
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 2] = gateway_config->system_server_port & 0xFF;
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 3] = gateway_config->heartbeat_interval >> 8;
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 4] = gateway_config->heartbeat_interval & 0xFF;
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 5] = gateway_config->report_interval >> 8;
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 6] = gateway_config->report_interval & 0xFF;
            udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 7] = gateway_config->config_lora.loraFreqGroup;
            memcpy(&udp_message->service_data->business_data_payload[GATEWAY_ID_LENGTH + PROJECT_ID_MAX_LENGTH + SERVER_URL_MAX_LENGTH + 8], gateway_config->config_time, 6);
            break;
        case SERVICE_TYPE_GATEWAY_CONFIG_NODE_REPORT:
            config_node_list_t* config_node_list = RT_NULL;
            get_business_data_cache(udp_message->transport_udp_data->event_id, (void**)&config_node_list);
            if(config_node_list == RT_NULL){
                rt_kprintf("Error: Failed to get config_node_list from cache\n");
                ret = -RT_ENOMEM;
                break;
            }

            udp_message->service_data->business_data_payload_length = config_node_list->nodes_count * (NODE_ID_LENGTH + 5) + 7;
            udp_message->service_data->business_data_payload = (uint8_t*)rt_malloc(udp_message->service_data->business_data_payload_length);
            if(udp_message->service_data->business_data_payload == RT_NULL){
                rt_kprintf("Error: Failed to malloc memory for business data payload\n");
                ret = -RT_ENOMEM;
                break;
            }
            udp_message->service_data->business_data_payload[0] = config_node_list->nodes_count;
            for(uint8_t i = 0; i < config_node_list->nodes_count; i++){
                udp_message->service_data->business_data_payload[i*(NODE_ID_LENGTH + 5) + 1] = config_node_list->config_nodes[i].node_type;
                rt_memcpy(&udp_message->service_data->business_data_payload[i*(NODE_ID_LENGTH + 5) + 2], config_node_list->config_nodes[i].node_id, NODE_ID_LENGTH);
                udp_message->service_data->business_data_payload[i*(NODE_ID_LENGTH + 5) + 2 + NODE_ID_LENGTH] = (uint8_t)(config_node_list->config_nodes[i].report_interval & 0xFF);
                udp_message->service_data->business_data_payload[i*(NODE_ID_LENGTH + 5) + 3 + NODE_ID_LENGTH] = (uint8_t)(config_node_list->config_nodes[i].report_interval >> 8);
                udp_message->service_data->business_data_payload[i*(NODE_ID_LENGTH + 5) + 4 + NODE_ID_LENGTH] = config_node_list->config_nodes[i].sensors_count;
                udp_message->service_data->business_data_payload[i*(NODE_ID_LENGTH + 5) + 5 + NODE_ID_LENGTH] = config_node_list->config_nodes[i].devices_count;
            }
            memcpy(&udp_message->service_data->business_data_payload[config_node_list->nodes_count*(NODE_ID_LENGTH + 5) + 1], config_node_list->config_time, 6);
            break;
        case SERVICE_TYPE_GATEWAY_CONFIG_DEVICE_REPORT:
            config_device_list_t *config_device_list;
            get_business_data_cache(udp_message->transport_udp_data->event_id, (void**)&config_device_list);        
            if(config_device_list == RT_NULL){
                rt_kprintf("Error: Failed to get config_device_report from cache\n");
                ret = -RT_ENOMEM;
                break;
            }
            
            uint8_t device_config_length = 17 + COMMAND_HEX_MAX_LENGTH * 3 + 6;
            udp_message->service_data->business_data_payload_length = NODE_ID_LENGTH + 1 + config_device_list->device_count * device_config_length;

            udp_message->service_data->business_data_payload = (uint8_t*)rt_malloc(udp_message->service_data->business_data_payload_length);
            if(udp_message->service_data->business_data_payload == RT_NULL){
                rt_kprintf("Error: Failed to allocate memory for device report payload, size: %d\n", udp_message->service_data->business_data_payload_length);
                ret = -RT_ENOMEM;
                break;
            }

            rt_memcpy(udp_message->service_data->business_data_payload, config_device_list->node_id, NODE_ID_LENGTH); 
            udp_message->service_data->business_data_payload[NODE_ID_LENGTH] = config_device_list->device_count; 

            uint8_t actual_device_count = (config_device_list->device_count < MAX_CHANNELS) ? config_device_list->device_count : MAX_CHANNELS;
            
            for(uint8_t i = 0; i < actual_device_count; i++){ 
                // 写入设备配置项
                udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*device_config_length] = config_device_list->devices[i].channel; 
                udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*device_config_length + 1] = config_device_list->devices[i].switch_type; 
                udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*device_config_length + 2] = (uint8_t)(config_device_list->devices[i].device_type >> 8); 
                udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*device_config_length + 3] = (uint8_t)(config_device_list->devices[i].device_type & 0xFF); 
                rt_memcpy(&udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*device_config_length + 4], config_device_list->devices[i].device_code, DEVICE_CODE_MAX_LENGTH); 
                rt_memcpy(&udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*device_config_length + 4 + DEVICE_CODE_MAX_LENGTH], config_device_list->devices[i].on_hex, COMMAND_HEX_MAX_LENGTH); 
                rt_memcpy(&udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*device_config_length + 4 + DEVICE_CODE_MAX_LENGTH + COMMAND_HEX_MAX_LENGTH], config_device_list->devices[i].off_hex, COMMAND_HEX_MAX_LENGTH); 
                rt_memcpy(&udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*device_config_length + 4 + DEVICE_CODE_MAX_LENGTH + 2*COMMAND_HEX_MAX_LENGTH], config_device_list->devices[i].set_hex, COMMAND_HEX_MAX_LENGTH); 
            }
            memcpy(&udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + actual_device_count * device_config_length + 4 + DEVICE_CODE_MAX_LENGTH + 2*COMMAND_HEX_MAX_LENGTH], config_device_list->config_time, 6);
            break;
        case SERVICE_TYPE_GATEWAY_CONFIG_SENSOR_REPORT:
            config_sensor_list_t *config_sensor_list = NULL;
            get_business_data_cache(udp_message->transport_udp_data->event_id, (void**)&config_sensor_list);
            if(config_sensor_list == RT_NULL){
                rt_kprintf("Error: Failed to get config_sensor_list pointer from cache\n");
                ret = -RT_ENOMEM;
                break;
            }

            udp_message->service_data->business_data_payload_length = NODE_ID_LENGTH + 1 + config_sensor_list->sensor_count * (COMMAND_HEX_MAX_LENGTH + DEVICE_CODE_MAX_LENGTH + 3 ) + 6;
            udp_message->service_data->business_data_payload = (uint8_t*)rt_malloc(udp_message->service_data->business_data_payload_length);
            if(udp_message->service_data->business_data_payload == RT_NULL){
                rt_kprintf("Error: Failed to allocate memory for sensor report payload, size: %d\n", udp_message->service_data->business_data_payload_length);
                ret = -RT_ENOMEM;
                break;
            }
            rt_memcpy(udp_message->service_data->business_data_payload, config_sensor_list->node_id, NODE_ID_LENGTH); 
            udp_message->service_data->business_data_payload[NODE_ID_LENGTH] = config_sensor_list->sensor_count;
            for(uint8_t i = 0; i < config_sensor_list->sensor_count; i++){
                udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(COMMAND_HEX_MAX_LENGTH + DEVICE_CODE_MAX_LENGTH + 3 )] = config_sensor_list->sensors[i].channel;
                udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(COMMAND_HEX_MAX_LENGTH + DEVICE_CODE_MAX_LENGTH + 3 ) + 1] = (uint8_t)(config_sensor_list->sensors[i].sensor_type >> 8);
                udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(COMMAND_HEX_MAX_LENGTH + DEVICE_CODE_MAX_LENGTH + 3 ) + 2] = (uint8_t)(config_sensor_list->sensors[i].sensor_type & 0xFF);
                rt_memcpy(&udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(COMMAND_HEX_MAX_LENGTH + DEVICE_CODE_MAX_LENGTH + 3 ) + 3], config_sensor_list->sensors[i].sensor_code, DEVICE_CODE_MAX_LENGTH);
                rt_memcpy(&udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i*(COMMAND_HEX_MAX_LENGTH + DEVICE_CODE_MAX_LENGTH + 3 ) + 3 + DEVICE_CODE_MAX_LENGTH], config_sensor_list->sensors[i].read_hex, COMMAND_HEX_MAX_LENGTH);
			}
            memcpy(&udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + config_sensor_list->sensor_count * (COMMAND_HEX_MAX_LENGTH + DEVICE_CODE_MAX_LENGTH + 3 )], config_sensor_list->config_time, 6);
            break;
        case SERVICE_TYPE_GATEWAY_SENSOR_DATA_REPORT:
            // get business data from cache
            service_data_sensor_data_report_t *sensor_data_report = RT_NULL;
            rt_err_t cache_result = get_business_data_cache(udp_message->transport_udp_data->event_id, (void**)&sensor_data_report);
            if(cache_result != RT_EOK || sensor_data_report == RT_NULL){
                rt_kprintf("Error: Failed to get sensor_data_report from cache, event_id: %d\n", udp_message->transport_udp_data->event_id);
                ret = -RT_ENOMEM;
                break;
            }
            uint16_t payload_len = NODE_ID_LENGTH + 8 + sensor_data_report->sensor_data_report_items_length * 6;
            udp_message->service_data->business_data_payload = (uint8_t*)rt_malloc(payload_len);
            if(udp_message->service_data->business_data_payload == RT_NULL){
                rt_kprintf("Error: Failed to allocate memory for sensor data payload\n");
                ret = -RT_ENOMEM;
                break;
            }
            rt_memset(udp_message->service_data->business_data_payload, 0, payload_len);

            rt_memcpy(&udp_message->service_data->business_data_payload[0], sensor_data_report->node_id, NODE_ID_LENGTH);
            rt_memcpy(&udp_message->service_data->business_data_payload[NODE_ID_LENGTH], sensor_data_report->collect_time, sizeof(sensor_data_report->collect_time));
            udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 6] = sensor_data_report->collect_response;
            udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 7] = sensor_data_report->sensor_data_report_items_length;
            for(uint8_t i = 0; i < sensor_data_report->sensor_data_report_items_length; i++){
              udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 8 + i * 6]     = sensor_data_report->sensor_data_report_items[i].channel;
              udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 8 + i * 6 + 1] = sensor_data_report->sensor_data_report_items[i].channel_result;
							rt_memcpy(&udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 8 + i * 6 + 2], sensor_data_report->sensor_data_report_items[i].value, sizeof(sensor_data_report->sensor_data_report_items[i].value) > 4 ? 4 : sizeof(sensor_data_report->sensor_data_report_items[i].value));
            }
            if(sensor_data_report->sensor_data_report_items_length > 0){
                rt_free(sensor_data_report->sensor_data_report_items);
            }
            udp_message->service_data->business_data_payload_length = payload_len;
            break;
        case SERVICE_TYPE_GATEWAY_CONFIG_RESPONSE:
        case SERVICE_TYPE_GATEWAY_CONFIG_NODE_RESPONSE:
        case SERVICE_TYPE_GATEWAY_CONFIG_SENSOR_RESPONSE:
        case SERVICE_TYPE_GATEWAY_CONFIG_DEVICE_RESPONSE:
        case SERVICE_TYPE_GATEWAY_CONFIG_TASK_RESPONSE:
            service_data_config_response_t* config_response = RT_NULL;
            cache_result = get_business_data_cache(udp_message->transport_udp_data->event_id, (void**)&config_response);
            if(cache_result != RT_EOK || config_response == RT_NULL){
                rt_kprintf("Error: Failed to get config_response from cache, event_id: %d\n", udp_message->transport_udp_data->event_id);
                ret = -RT_ENOMEM;
                break;
            }
            payload_len = (1 + strlen(config_response->message)) > MAX_RESPONSE_MESSAGE_LENGTH ? MAX_RESPONSE_MESSAGE_LENGTH : (1 + strlen(config_response->message));
            udp_message->service_data->business_data_payload = (uint8_t*)rt_malloc(payload_len);
            if(udp_message->service_data->business_data_payload == RT_NULL){
                rt_kprintf("Error: Failed to allocate memory for config_response payload\n");
                ret = -RT_ENOMEM;
                break;
            }
            rt_memset(udp_message->service_data->business_data_payload, 0, payload_len);            
            //service_data_config_response_t* config_response = (service_data_config_response_t*)udp_message->service_data->business_data;
            udp_message->service_data->business_data_payload[0]     = config_response->result;
            rt_memcpy(&udp_message->service_data->business_data_payload[1], config_response->message, payload_len - 1);
            udp_message->service_data->business_data_payload_length = payload_len;
            break;
        case SERVICE_TYPE_GATEWAY_DEVICE_CONTROL_RESPONSE:
            /* device control response 
            service_data_gateway_device_control_response_t* device_control_response = (service_data_gateway_device_control_response_t*)udp_message->service_data->business_data;
            rt_memcpy(&udp_message->service_data->business_data_payload, device_control_response->node_id, NODE_ID_LENGTH);
            udp_message->service_data->business_data_payload[NODE_ID_LENGTH] = device_control_response->device_size;
            for(uint8_t i = 0; i < device_control_response->device_size; i++){
                udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i]     = device_control_response->device_control_response_items[i].channel;
                udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 1 + i + 1] = device_control_response->device_control_response_items[i].result;
            }
            udp_message->service_data->business_data_payload_length = NODE_ID_LENGTH + 1 + device_control_response->device_size * 2;    
            */
            // get business data from cache
            service_data_gateway_device_control_t *device_control = RT_NULL;
            cache_result = get_business_data_cache(udp_message->transport_udp_data->event_id, (void**)&device_control);
            if(cache_result != RT_EOK || device_control == RT_NULL){
                rt_kprintf("Error: Failed to get device_control from cache, event_id: %d\n", udp_message->transport_udp_data->event_id);
                ret = -RT_ENOMEM;
                break;
            }
            rt_memcpy(&udp_message->service_data->business_data_payload[0], device_control->node_id, NODE_ID_LENGTH);
            udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 6] = device_control->gateway_device_control_items_length;
            for(uint8_t i = 0; i < device_control->gateway_device_control_items_length; i++){
                udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 7 + i * 2]     = device_control->gateway_device_control_items[i].channel;
                udp_message->service_data->business_data_payload[NODE_ID_LENGTH + 7 + i * 2 + 1] = device_control->gateway_device_control_items[i].action;
            }
            udp_message->service_data->business_data_payload_length = NODE_ID_LENGTH + 6 + device_control->gateway_device_control_items_length * 2;
            break;
        case SERVICE_TYPE_GATEWAY_UPGRADE_STATUS:
            service_data_gateway_upgrade_status_t* upgrade_status;
            cache_result = get_business_data_cache(udp_message->transport_udp_data->event_id, (void**)&upgrade_status);
            if(cache_result != RT_EOK || upgrade_status == RT_NULL){
                rt_kprintf("Error: Failed to get upgrade_status from cache, event_id: %d\n", udp_message->transport_udp_data->event_id);
                ret = -RT_ENOMEM;
                break;
            }
            rt_memcpy(udp_message->service_data->business_data_payload, upgrade_status->upgrade_id, 64);
            udp_message->service_data->business_data_payload[64]     = upgrade_status->app_type;
            udp_message->service_data->business_data_payload[64 + 1] = upgrade_status->status;
            udp_message->service_data->business_data_payload[64 + 2] = upgrade_status->percent;
            rt_memcpy(&udp_message->service_data->business_data_payload[64 + 3], upgrade_status->node_id, NODE_ID_LENGTH);
            udp_message->service_data->business_data_payload_length = 67 + NODE_ID_LENGTH;
            break;
        case SERVICE_TYPE_GATEWAY_CONFIG_TASK_REPORT:
            config_gateway_task_list_t* config_task_list = RT_NULL;
            cache_result = get_business_data_cache(udp_message->transport_udp_data->event_id, (void**)&config_task_list);
            if(cache_result != RT_EOK || config_task_list == RT_NULL){
                rt_kprintf("Error: Failed to get config_task_list from cache, event_id: %d\n", udp_message->transport_udp_data->event_id);
                ret = -RT_EEMPTY;
                break;
            }
            udp_message->service_data->business_data_payload_length = config_task_list->task_count * sizeof(config_gateway_task_t);
            udp_message->service_data->business_data_payload = rt_malloc(udp_message->service_data->business_data_payload_length);
            if(udp_message->service_data->business_data_payload == RT_NULL){
                rt_kprintf("Error: Failed to malloc memory for business data payload, event_id: %d\n", udp_message->transport_udp_data->event_id);
                ret = -RT_ENOMEM;
                break;
            }
            udp_message->service_data->business_data_payload[0] = config_task_list->task_count;
            for(uint8_t i = 0; i < config_task_list->task_count; i++){
                udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 1]     = config_task_list->config_tasks[i].task_id;
                udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 2] = config_task_list->config_tasks[i].trigger_type;
                udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 3] = config_task_list->config_tasks[i].enabled;
                rt_memcpy(&udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 3], config_task_list->config_tasks[i].trigger_time, 3);
                udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 7] = config_task_list->config_tasks[i].trigger_size;
                for(uint8_t j = 0; j < config_task_list->config_tasks[i].trigger_size; j++){
                    udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 8 + j] = config_task_list->config_tasks[i].triggers[j].join_type;
                    rt_memcpy(&udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 9 + j], config_task_list->config_tasks[i].triggers[j].node_id, NODE_ID_LENGTH);
                    udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 9 + NODE_ID_LENGTH + j] = config_task_list->config_tasks[i].triggers[j].channel;
                    udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 10 + NODE_ID_LENGTH + j] = config_task_list->config_tasks[i].triggers[j].compare_type;
                    //udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 11 + j] = config_task_list->config_tasks[i].triggers[j].value >> 24;
                    //udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 12 + j] = config_task_list->config_tasks[i].triggers[j].value >> 16;
                    udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 11 + NODE_ID_LENGTH + j] = config_task_list->config_tasks[i].triggers[j].value >> 8;
                    udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 12 + NODE_ID_LENGTH + j] = config_task_list->config_tasks[i].triggers[j].value;
                }
                udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 13 + NODE_ID_LENGTH] = config_task_list->config_tasks[i].action_size;
                for(uint8_t j = 0; j < config_task_list->config_tasks[i].action_size; j++){
                    rt_memcpy(&udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 14 + NODE_ID_LENGTH + j], config_task_list->config_tasks[i].actions[j].node_id, NODE_ID_LENGTH);
                    udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 14 + NODE_ID_LENGTH*2+ j] = config_task_list->config_tasks[i].actions[j].channel;
                    udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 15 + NODE_ID_LENGTH*2+ j] = config_task_list->config_tasks[i].actions[j].action;
                    udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 16 + NODE_ID_LENGTH*2+ j] = config_task_list->config_tasks[i].actions[j].value >> 8;
                    udp_message->service_data->business_data_payload[i * sizeof(config_gateway_task_t) + 17 + NODE_ID_LENGTH*2+ j] = config_task_list->config_tasks[i].actions[j].value;
                }
            }
            break;
        default:
            rt_kprintf("Encode Info: transport_udp_service_encode service_type: %d with no service data\n", service_type);
            if(udp_message->service_data->business_data_payload != RT_NULL) {
                rt_free(udp_message->service_data->business_data_payload);
                udp_message->service_data->business_data_payload = RT_NULL;
            }
            udp_message->service_data->business_data_payload_length = 0;
            udp_message->service_data->business_data_payload = RT_NULL;
            ret = RT_EOK;
            break;
    }
    remove_business_data_cache(udp_message->transport_udp_data->event_id);
    udp_message->service_data->business_data = RT_NULL;
    return ret;
}