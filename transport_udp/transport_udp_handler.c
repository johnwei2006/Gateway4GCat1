#include "transport_udp_handler.h"
#include "transport_udp_message.h"
#include "transport_message_cache.h"
#include <rtthread.h>
#include <rtdevice.h>
#include "utils.h"
#include "../config/config_manager.h"
#include "../service_manager/service_manager_udp.h"
#include "../transport_udp_encoder/transport_udp_encoder.h"
#include "../transport_udp_encoder/transport_udp_service_encoder.h"
#include "../transport/transport_mem_pool_manager.h"

#define MODULE_NAME  "UDP_HANDLER"
#include "../utils/debug_log.h"
#include <rthw.h>

#define RAW_UDP_THREAD_STACK_SIZE  2048
#define RAW_UDP_THREAD_PRIORITY    11
#define RAW_UDP_THREAD_TIMESLICE   30

#define UPLINK_MESSAGE_CACHE_LIMIT 10
#define MUTEX_TIMEOUT_MS           100

rt_thread_t cloud_uplink_thread = RT_NULL;
rt_thread_t cloud_downlink_thread = RT_NULL;
rt_thread_t uplink_message_period_resend_thread = RT_NULL;

static rt_mutex_t cloud_uplink_mutex;
static rt_mutex_t cloud_downlink_mutex;

static void cloud_downlink_process(void* data){
    transport_udp_message_t 	*transport_udp_message	=	RT_NULL;
		raw_udp_message_t 				*raw_udp_message 				= RT_NULL;

    while(1)
    {
      raw_udp_message = RT_NULL;
      rt_err_t ret = get_cloud_downlink_message(&raw_udp_message);
      if (ret == RT_EOK && raw_udp_message != RT_NULL && raw_udp_message->payload_length > 0) {
        if(cloud_downlink_mutex != RT_NULL) {
          rt_err_t err = rt_mutex_take(cloud_downlink_mutex, MUTEX_TIMEOUT_MS); 
          if(err != RT_EOK) {
            LOG_W("Failed to take cloud_downlink_mutex\n");
            free_raw_udp_message(raw_udp_message);
            rt_thread_mdelay(10);
            continue;
          }
        }
        transport_udp_message = alloc_transport_udp_message();
        if (transport_udp_message == RT_NULL) {
          LOG_E("alloc transport_udp_message from pool failed\n");
          free_raw_udp_message(raw_udp_message);
          if(cloud_downlink_mutex != RT_NULL) {
            rt_mutex_release(cloud_downlink_mutex);
          }
          rt_thread_mdelay(10);
          continue;
        }

        transport_udp_decode(raw_udp_message, transport_udp_message);

        rt_kprintf("cloud_downlink_process decode data: %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% \r\n");
        rt_kprintf("service_type  : %d \r\n", transport_udp_message->transport_udp_data->service_type);
        rt_kprintf("message_id    : %d \r\n", transport_udp_message->transport_udp_data->message_id);
        rt_kprintf("event_id      : %d \r\n", transport_udp_message->transport_udp_data->event_id);
        rt_kprintf("gateway_id    : %s \r\n", transport_udp_message->transport_udp_data->gateway_id);
        rt_kprintf("node_id       : %s \r\n", transport_udp_message->transport_udp_data->node_id);
        rt_kprintf("payload_length: %d \r\n", transport_udp_message->service_data->business_data_payload_length);
        rt_kprintf("message size  : %d \r\n", raw_udp_message->payload_length);
        rt_kprintf("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\r\n");

        free_raw_udp_message(raw_udp_message);

        // 3. deal ack message
        if (transport_udp_message->transport_udp_data->service_type == SERVICE_TYPE_ACK) {
          handler_deal_ack_message(transport_udp_message);
          if(cloud_downlink_mutex != RT_NULL) {
            rt_mutex_release(cloud_downlink_mutex);
          }
          rt_thread_mdelay(10);
          continue;
        }
        // 4. send ack message to platform
        handler_send_ack_message(transport_udp_message);
        // 5. decode service data
        int svc_ret = transport_udp_service_decode(transport_udp_message, &transport_udp_message->service_data->business_data);
        if(svc_ret != RT_EOK){
          rt_kprintf("transport_udp_service_decode error. \r\n");
          release_transport_udp_message(transport_udp_message);
          if(cloud_downlink_mutex != RT_NULL) {
            rt_mutex_release(cloud_downlink_mutex);
          }
          rt_thread_mdelay(10);
          continue;
        }
        // 6. service process
        svc_ret = service_manager_process_downlink(transport_udp_message, transport_udp_message->service_data->business_data);
        if(svc_ret != RT_EOK){
          rt_kprintf("service_manager_process_downlink error. \r\n");
        } 

        release_transport_udp_message(transport_udp_message);
        if(cloud_downlink_mutex != RT_NULL) {
          rt_mutex_release(cloud_downlink_mutex);
        }
			}
      rt_thread_mdelay(10);
    }
}

static void cloud_uplink_process(void* data){
    transport_udp_message_t *transport_udp_message	= RT_NULL;
		raw_udp_message_t 			*raw_udp_message 				= RT_NULL;

  	while(1)
    {  
       transport_udp_message = RT_NULL;
			 rt_err_t result = get_gateway_uplink_message(&transport_udp_message);
       if (transport_udp_message == RT_NULL) {
				 rt_thread_mdelay(50);
				 continue;
			 }

       if(cloud_uplink_mutex != RT_NULL) {
         rt_err_t err = rt_mutex_take(cloud_uplink_mutex, MUTEX_TIMEOUT_MS); 
         if(err != RT_EOK) {
           LOG_W("Failed to take cloud_uplink_mutex\n");
           free_transport_udp_message(transport_udp_message);
           rt_thread_mdelay(10);
           continue;
         }
       }
       //2. setting transport_udp_header and transport_udp_data
				transport_udp_message->transport_udp_header->transport_data_encoding = TRANSPORT_DATA_ENCODING_BINARY;
				transport_udp_message->transport_udp_header->transport_version       = TRANSPORT_VERSION_1;

				transport_udp_message->transport_udp_data->message_id = generate_rand_short();
				// 2.1 setting gateway_id, node_id, message_create_time
				config_gateway_serial_t gateway_config;
				read_config_gateway_serial(&gateway_config);
				rt_memcpy(transport_udp_message->transport_udp_data->gateway_id, gateway_config.gateway_id, GATEWAY_ID_LENGTH);
				//const char *timeStamp = getTimeStamp();
				//rt_memset(transport_udp_message->transport_udp_data->node_id, 0, 4);
				transport_udp_message->transport_udp_data->service_data_encoding         = SERVICE_DATA_ENCODING_BINARY;
				transport_udp_message->transport_udp_data->service_data_protocol_version = SERVICE_DATA_PROTOCOL_VERSION_1;
				transport_udp_message->transport_udp_data->service_data_encryption       = SERVICE_DATA_ENCRYPTION_NONE;

        //3. service encode transport_udp_message  service_data->business_data to service_data->business_data_payload and set service_data->business_data_payload_length
				int encode_result = transport_udp_service_encode(transport_udp_message);
				if(encode_result != RT_EOK){
					rt_kprintf("Error: transport_udp_service_encode failed, ret: %d \n", encode_result);
					free_transport_udp_message(transport_udp_message);
					transport_udp_message = RT_NULL;
					if(cloud_uplink_mutex != RT_NULL) {
						rt_mutex_release(cloud_uplink_mutex);
					}
					rt_thread_mdelay(10);
					continue;
				}
				transport_udp_message->transport_udp_header->transport_data_length = transport_udp_message->service_data->business_data_payload_length + TRANSPORT_DATA_FIX_LENGTH;
					
        rt_kprintf("\r\ncloud_uplink_process send data: ++++++++++++++++++++++++++++++++++++++++++ \r\n");
        rt_kprintf("service_type  : %d \r\n", transport_udp_message->transport_udp_data->service_type);
        rt_kprintf("message_id    : %d \r\n", transport_udp_message->transport_udp_data->message_id);
        rt_kprintf("event_id      : %d \r\n", transport_udp_message->transport_udp_data->event_id);
        rt_kprintf("gateway_id    : %s \r\n", transport_udp_message->transport_udp_data->gateway_id);
        rt_kprintf("node_id       : %s \r\n", transport_udp_message->transport_udp_data->node_id);
        rt_kprintf("payload_length: %d \r\n", transport_udp_message->service_data->business_data_payload_length);
        //rt_kprintf("message size  : %d \r\n", raw_udp_message->payload_length);
        rt_kprintf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ \r\n");

        //4. put uplink message to cache, waiting for ack message from platform
        if(transport_udp_message->transport_udp_data->service_type != SERVICE_TYPE_HEARTBEAT && transport_udp_message->transport_udp_data->service_type != SERVICE_TYPE_ACK){
				  put_uplink_message_cache(transport_udp_message);
          if(get_uplink_message_cache_size() >= UPLINK_MESSAGE_CACHE_LIMIT){
            LOG_W("uplink message cache full (%d), triggering flow control\n", get_uplink_message_cache_size());
            rt_thread_mdelay(100);
          }
        }
                   
        //5. encode transport_udp_message to raw_udp_message
        raw_udp_message = (raw_udp_message_t*)rt_malloc(sizeof(raw_udp_message_t));
        if (raw_udp_message == RT_NULL) {
          LOG_E("Failed to alloc raw_udp_message!\n");
          remove_uplink_message_cache(transport_udp_message->transport_udp_data->message_id);
          free_transport_udp_message(transport_udp_message);
          if(cloud_uplink_mutex != RT_NULL) {
             rt_mutex_release(cloud_uplink_mutex);
          }
          rt_thread_mdelay(10);
          continue;
        }
        rt_memset(raw_udp_message, 0, sizeof(raw_udp_message_t));
        raw_udp_message->payload_length = TRANSPORT_HEADER_FIX_LENGTH + transport_udp_message->transport_udp_header->transport_data_length + 1;
        raw_udp_message->payload = (char*)rt_malloc(raw_udp_message->payload_length);
        if(raw_udp_message->payload == RT_NULL){
          LOG_E("Failed to alloc raw_udp_payload!\n");
          rt_free(raw_udp_message);
          raw_udp_message = RT_NULL;
          remove_uplink_message_cache(transport_udp_message->transport_udp_data->message_id);
          free_transport_udp_message(transport_udp_message);
          if(cloud_uplink_mutex != RT_NULL) {
             rt_mutex_release(cloud_uplink_mutex);
          }
          rt_thread_mdelay(10);
          continue;
        }
        rt_memset(raw_udp_message->payload, 0, raw_udp_message->payload_length);

				transport_udp_encode(transport_udp_message, raw_udp_message);
        rt_kprintf("message size  : %d \r\n", raw_udp_message->payload_length);
        //6. put raw_udp_message to cloud uplink message queue
				put_cloud_uplink_message(&raw_udp_message);

        //7. free gateway_uplink_message, ignore: after ack message processed in remove_uplink_message_cache
        if(transport_udp_message != RT_NULL && transport_udp_message->transport_udp_data != RT_NULL && transport_udp_message->transport_udp_data->service_type == SERVICE_TYPE_HEARTBEAT){
          free_transport_udp_message(transport_udp_message);
          transport_udp_message = RT_NULL;
        }
        if(cloud_uplink_mutex != RT_NULL) {
          rt_mutex_release(cloud_uplink_mutex);
        }
				rt_thread_mdelay(50);
		}
}

static int handler_deal_ack_message(transport_udp_message_t *transport_udp_message)
{
    //1. check ack message
    if (transport_udp_message->transport_udp_data->service_type != SERVICE_TYPE_ACK) {
        rt_kprintf("Warning: receive invalid ack message, service_type: %d \n", transport_udp_message->transport_udp_data->service_type);
        return RT_EINVAL;   
    }
    //2. process ack message
    rt_kprintf("Info: receive ack message, message_id: %d \n", transport_udp_message->transport_udp_data->message_id);
    rt_err_t result = remove_uplink_message_cache(transport_udp_message->transport_udp_data->message_id);
    if(result != RT_EOK){
        rt_kprintf("Warning: remove ack message cache failed, message_id: %d \n", transport_udp_message->transport_udp_data->message_id);
        return result;
    }
    //2.2 free ack message (pool-allocated, use release)
    release_transport_udp_message(transport_udp_message);
    return RT_EOK;
}

static int handler_send_ack_message(transport_udp_message_t *transport_udp_message)
{
    transport_udp_message_t *ack_transport_udp_message = RT_NULL;
    raw_udp_message_t       *ack_raw_udp_message       = RT_NULL;
    
    ack_transport_udp_message = alloc_transport_udp_message();
    if(ack_transport_udp_message == RT_NULL){
        LOG_E("alloc ack_transport_udp_message from pool failed\n");
        return RT_ENOMEM;
    }

    ack_transport_udp_message->transport_udp_header->transport_data_length = TRANSPORT_HEADER_FIX_LENGTH + TRANSPORT_DATA_FIX_LENGTH + 1;
    ack_transport_udp_message->transport_udp_header->transport_data_encoding = transport_udp_message->transport_udp_header->transport_data_encoding;
    ack_transport_udp_message->transport_udp_header->transport_version = transport_udp_message->transport_udp_header->transport_version;
    
    ack_transport_udp_message->transport_udp_data->message_id = transport_udp_message->transport_udp_data->message_id;
    rt_memcpy(ack_transport_udp_message->transport_udp_data->gateway_id, transport_udp_message->transport_udp_data->gateway_id, GATEWAY_ID_LENGTH);
    rt_memcpy(ack_transport_udp_message->transport_udp_data->node_id, transport_udp_message->transport_udp_data->node_id, NODE_ID_LENGTH);
    rt_memcpy(ack_transport_udp_message->transport_udp_data->message_create_time, transport_udp_message->transport_udp_data->message_create_time, CREATE_TIME_LENGTH);
    ack_transport_udp_message->transport_udp_data->service_type = SERVICE_TYPE_ACK;
    ack_transport_udp_message->transport_udp_data->service_version = transport_udp_message->transport_udp_data->service_version;
    ack_transport_udp_message->transport_udp_data->service_data_encoding = transport_udp_message->transport_udp_data->service_data_encoding;
    ack_transport_udp_message->transport_udp_data->service_data_protocol_version = transport_udp_message->transport_udp_data->service_data_protocol_version;
    ack_transport_udp_message->transport_udp_data->service_data_encryption = transport_udp_message->transport_udp_data->service_data_encryption;
    ack_transport_udp_message->transport_udp_data->event_id = transport_udp_message->transport_udp_data->event_id;
    
    ack_transport_udp_message->service_data->business_data = RT_NULL;
    ack_transport_udp_message->service_data->business_data_payload = RT_NULL;
    ack_transport_udp_message->service_data->business_data_payload_length = 0;
    
    ack_raw_udp_message = (raw_udp_message_t*)rt_malloc(sizeof(raw_udp_message_t));
    if(ack_raw_udp_message == RT_NULL){
        LOG_E("alloc ack_raw_udp_message failed\n");
        release_transport_udp_message(ack_transport_udp_message);
        return RT_ENOMEM;
    }
    rt_memset(ack_raw_udp_message, 0, sizeof(raw_udp_message_t));
    
    ack_raw_udp_message->payload_length = TRANSPORT_HEADER_FIX_LENGTH + ack_transport_udp_message->transport_udp_header->transport_data_length + 1;
    ack_raw_udp_message->payload = (char*)rt_malloc(ack_raw_udp_message->payload_length);
    if(ack_raw_udp_message->payload == RT_NULL){
        LOG_E("alloc ack_payload failed\n");
        rt_free(ack_raw_udp_message);
        release_transport_udp_message(ack_transport_udp_message);
        return RT_ENOMEM;
    }
    rt_memset(ack_raw_udp_message->payload, 0, ack_raw_udp_message->payload_length);
    
    transport_udp_encode(ack_transport_udp_message, ack_raw_udp_message);
    
    rt_kprintf("send ack message to platform: ++++++++++++++++++++++++++++++++++++++++++ \r\n");
    rt_kprintf("service_type  : %d \r\n", ack_transport_udp_message->transport_udp_data->service_type);
    rt_kprintf("message_id    : %d \r\n", ack_transport_udp_message->transport_udp_data->message_id);
    rt_kprintf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ \r\n");
    
    put_cloud_uplink_message(&ack_raw_udp_message);
    release_transport_udp_message(ack_transport_udp_message);
    return RT_EOK;
}

static void uplink_message_period_resend_process(void* data){
    raw_udp_message_t *raw_udp_message = RT_NULL;
    while(1){
        rt_thread_mdelay(UPLINK_MESSAGE_PERIOD_RESEND_INTERVAL);
        
        rt_mutex_take(uplink_message_cache_mutex, RT_WAITING_FOREVER);
        uint8_t cache_size = cps_list_size(uplink_message_cache_head_node);
        if(cache_size == 0 || current_resend_uplink_message_index >= cache_size){
            current_resend_uplink_message_index = 0;
            rt_mutex_release(uplink_message_cache_mutex);
            continue;
        }
        current_resend_uplink_message_index++;
        transport_udp_message_t *transport_udp_message = (transport_udp_message_t *)cps_list_get_node(uplink_message_cache_head_node, current_resend_uplink_message_index);
        if(transport_udp_message == RT_NULL){
            current_resend_uplink_message_index = 0;
            rt_mutex_release(uplink_message_cache_mutex);
            continue;
        }
        rt_kprintf("Info: resend message, message_id: %d \n", transport_udp_message->transport_udp_data->message_id);
        raw_udp_message = (raw_udp_message_t*)rt_malloc(sizeof(raw_udp_message_t));
        if(raw_udp_message == RT_NULL){
            rt_mutex_release(uplink_message_cache_mutex);
            continue;
        }
        rt_memset(raw_udp_message, 0, sizeof(raw_udp_message_t));
        uint16_t payload_len = TRANSPORT_HEADER_FIX_LENGTH + transport_udp_message->transport_udp_header->transport_data_length + 1;
        raw_udp_message->payload_length = payload_len;
        raw_udp_message->payload = (char*)rt_malloc(payload_len);
        if(raw_udp_message->payload == RT_NULL){
            rt_free(raw_udp_message);
            rt_mutex_release(uplink_message_cache_mutex);
            continue;
        }
        rt_memset(raw_udp_message->payload, 0, payload_len);
        transport_udp_encode(transport_udp_message, raw_udp_message);
        rt_mutex_release(uplink_message_cache_mutex);
        put_cloud_uplink_message(&raw_udp_message);
    }
}
int transport_udp_handler_init()
{
	  rt_err_t ret = RT_EOK;

    cloud_uplink_mutex = rt_mutex_create("cloud_uplink_mutex", RT_IPC_FLAG_FIFO);
    if (cloud_uplink_mutex == RT_NULL) {
      rt_kprintf("Failed to create cloud_uplink_mutex\n");
      ret = RT_ERROR;
      goto err_exit;
    }
    cloud_downlink_mutex = rt_mutex_create("cloud_downlink_mutex", RT_IPC_FLAG_FIFO);
    if (cloud_downlink_mutex == RT_NULL) {
      rt_kprintf("Failed to create cloud_downlink_mutex\n");
      rt_mutex_delete(cloud_uplink_mutex);
      cloud_uplink_mutex = RT_NULL;
      ret = RT_ERROR;
      goto err_exit;
    }

    if(cloud_uplink_thread == RT_NULL){
        cloud_uplink_thread = rt_thread_create("cloud_uplink_thread", cloud_uplink_process, RT_NULL, 1536, RAW_UDP_THREAD_PRIORITY, RAW_UDP_THREAD_TIMESLICE);
        if (cloud_uplink_thread != RT_NULL){
            rt_thread_startup(cloud_uplink_thread);
        } else {
					  rt_kprintf("Failed to create cloud_uplink_thread!\n");
            ret = RT_ERROR;
            goto err_destroy_mutex;
				}
    }
    if(cloud_downlink_thread == RT_NULL){
        cloud_downlink_thread = rt_thread_create("cloud_downlink_thread", cloud_downlink_process, RT_NULL, 1536, RAW_UDP_THREAD_PRIORITY, RAW_UDP_THREAD_TIMESLICE);
        if (cloud_downlink_thread != RT_NULL)
        {
            rt_thread_startup(cloud_downlink_thread);
        } else{					
					  ret = RT_ERROR;
            goto err_destroy_uplink;
        }
		 }
     if(uplink_message_period_resend_thread == RT_NULL){
        uplink_message_period_resend_thread = rt_thread_create("uplink_message_period_resend_thread", uplink_message_period_resend_process, RT_NULL, 1024, RAW_UDP_THREAD_PRIORITY, RAW_UDP_THREAD_TIMESLICE);
        if (uplink_message_period_resend_thread != RT_NULL)
        {
            //rt_thread_startup(uplink_message_period_resend_thread);
        } else{
            rt_kprintf("Failed to create uplink_message_period_resend_thread!\n");
            ret = RT_ERROR;
            goto err_destroy_uplink;
        }
			}

      rt_kprintf("transport_udp_handler_init success!\n");
      return RT_EOK;

err_destroy_uplink:
    if (cloud_downlink_thread != RT_NULL)
    {
        rt_thread_delete(cloud_downlink_thread);
        cloud_downlink_thread = RT_NULL;
    }
    if (cloud_uplink_thread != RT_NULL)
    {
        rt_thread_delete(cloud_uplink_thread);
        cloud_uplink_thread = RT_NULL;
    }
err_destroy_mutex:
    if (cloud_downlink_mutex != RT_NULL) {
      rt_mutex_delete(cloud_downlink_mutex);
      cloud_downlink_mutex = RT_NULL;
    }
    if (cloud_uplink_mutex != RT_NULL) {
      rt_mutex_delete(cloud_uplink_mutex);
      cloud_uplink_mutex = RT_NULL;
    }
err_exit:
    rt_kprintf("transport_udp_handler_init failed! ret = %d\n", ret);
    return ret;	
}
// system auto queue_init
INIT_APP_EXPORT(transport_udp_handler_init);