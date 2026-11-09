#include "service_gateway_device_control.h"
#include "../config/config_manager.h"
#include <crc16.h>

rt_err_t service_execute_gateway_device_control(void* transport_udp_message, void* business_data){
    rt_err_t return_value = RT_EOK;
     if(transport_udp_message == RT_NULL)
       return_value = RT_EINVAL;

    transport_udp_message_t *transport_udp_message_ptr = (transport_udp_message_t*)transport_udp_message;
    service_data_gateway_device_control_t *device_control = RT_NULL;
    get_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id, (void**)&device_control);
    if(device_control == RT_NULL)       
        return_value = RT_EINVAL;

    modbus_request_message_t *modbus_request_message = (modbus_request_message_t*)rt_malloc(sizeof(modbus_request_message_t));
    modbus_response_message_t modbus_response_message =  {0};
    for(uint8_t i = 0; i < device_control->gateway_device_control_items_length; i++){
        service_data_gateway_device_control_item_t* item = &device_control->gateway_device_control_items[i];
        rt_kprintf("Device Control - Channel: %d, Action: %d, Value: %u\n", item->channel, item->action, *((uint32_t*)item->value));
        // Here you can add the actual device control logic based on channel, action, and value
        //1. identify if it's virtual node or real node by control->node_id
        //2. find the device configuration by node_id and channel
        config_node_serial_t config_node = {0};
        uint8_t  node_id[11] = {0};
        rt_memcpy(node_id, device_control->node_id, NODE_ID_LENGTH);
        flash_status result = read_config_node_serial(&config_node, node_id);
        if(result != FLASH_OK){
            rt_kprintf("Node ID %s not found!\n", device_control->node_id);
            return_value = RT_EINVAL;
            break;
        }
        config_device_t config_device = {0};
        result = read_config_device(&config_device, node_id, config_node.device_block_numbers[i]);
        if(result != FLASH_OK){
            rt_kprintf("Device on Block Number %d not found for Node ID %s!\n", config_node.device_block_numbers[i], device_control->node_id);
            return_value = RT_EINVAL;
            break;
        }

        //3. execute the action (open, close, stop, set) with the provided value
        switch(item->action){
            case DEVICE_CONTROL_ACTION_ON:
                rt_kprintf("Executing ON command for Channel %d on Node ID %s\n", item->channel, device_control->node_id);
                return_value = modbus_request(config_device.on_hex, sizeof(config_device.on_hex), modbus_request_message, &modbus_response_message);
                if(return_value != RT_EOK){
                    rt_kprintf("Failed to send ON command for Channel %d on Node ID %s!\n", item->channel, device_control->node_id);
                    break;
                }
                break;
            case DEVICE_CONTROL_ACTION_OFF:
                rt_kprintf("Executing OFF command for Channel %d on Node ID %s\n", item->channel, device_control->node_id);            
                return_value = modbus_request(config_device.off_hex, sizeof(config_device.off_hex), modbus_request_message,&modbus_response_message);
                if(return_value != RT_EOK){
                    rt_kprintf("Failed to send OFF command for Channel %d on Node ID %s!\n", item->channel, device_control->node_id);
                    break;
                }
                break;
            case DEVICE_CONTROL_ACTION_STOP:
                rt_kprintf("Executing STOP command for Channel %d on Node ID %s\n", item->channel, device_control->node_id);
                return_value = modbus_request(config_device.set_hex, sizeof(config_device.set_hex), modbus_request_message, &modbus_response_message);
                if(return_value != RT_EOK){
                    rt_kprintf("Failed to send STOP command for Channel %d on Node ID %s!\n", item->channel, device_control->node_id);
                    break;    
                }
                break;
            case DEVICE_CONTROL_ACTION_SET:
                rt_kprintf("Executing SET command for Channel %d on Node ID %s with Value %u\n", item->channel, device_control->node_id, *((uint32_t*)item->value));
                //1. replace the placeholder in set_hex_command with the actual value
                char set_hex_command[COMMAND_HEX_MAX_LENGTH];
                //strcpy(set_hex_command, config_device->set_hex_command);
                snprintf(set_hex_command, sizeof(set_hex_command), config_device.set_hex, *((uint32_t*)item->value));
                //2. crc16
                uint16_t crc = crc16_cyc_cal(CRC16_INIT_VAL, (uint8_t*)set_hex_command, sizeof(set_hex_command));
                set_hex_command[sizeof(set_hex_command) - 2] = crc;
                set_hex_command[sizeof(set_hex_command) - 1] = crc >> 8;
                
                //3. send the set command
                return_value = modbus_request_setting(set_hex_command, sizeof(set_hex_command), item->value, modbus_request_message,&modbus_response_message);
                if(return_value != RT_EOK){
                    rt_kprintf("Failed to send SET command for Channel %d on Node ID %s!\n", item->channel, device_control->node_id);
                    break;
                }
                break;
            default:
                rt_kprintf("Unknown action %d for Channel %d on Node ID %s!\n", item->action, item->channel, device_control->node_id); 
        }

        item->action = device_control_is_success(return_value, modbus_request_message, &modbus_response_message);
    }

    // After processing all control items, report the results to platform, same event_id with the request
	  transport_udp_message_ptr->transport_udp_data->service_type  			= SERVICE_TYPE_GATEWAY_DEVICE_CONTROL_RESPONSE;
    put_gateway_uplink_message(transport_udp_message_ptr);
    // remove the business data cache & modbus request message after use to avoid memory leak
    remove_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id);
    rt_free(modbus_request_message);
    return return_value;
}
			 
service_gateway_device_control_t* service_gateway_device_control_create(){
    service_gateway_device_control_t* service_gateway_device_control = (service_gateway_device_control_t*)rt_malloc(sizeof(service_gateway_device_control_t));
    service_gateway_device_control->base.service_type 		= SERVICE_TYPE_GATEWAY_DEVICE_CONTROL;
    service_gateway_device_control->base.service_execute 	= service_execute_gateway_device_control;
    return service_gateway_device_control;
}

bool device_control_is_success(rt_err_t return_value, modbus_request_message_t* modbus_request_message, modbus_response_message_t* modbus_response_message){
    return return_value == RT_EOK;
}