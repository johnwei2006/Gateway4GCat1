#include "transport_message_queue.h"
#include "../transport_lora/lora_message.h"
#include <rtthread.h>

// Message queue definitions
rt_mq_t lora_uplink_mq     = RT_NULL;
rt_mq_t lora_downlink_mq   = RT_NULL;
rt_mq_t node_downlink_mq   = {0};
rt_mq_t gateway_uplink_mq  = RT_NULL;
rt_mq_t gateway_downlink_mq = RT_NULL;
rt_mq_t cloud_uplink_mq    = {0};
rt_mq_t cloud_downlink_mq  = {0};

// 1. lora downlink message queue
rt_err_t put_lora_downlink_message(lora_message_t *lora_tx_message) {
    if (lora_tx_message == RT_NULL || lora_downlink_mq == RT_NULL) {
        rt_kprintf("put_lora_downlink_message: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    return rt_mq_send(lora_downlink_mq, &lora_tx_message, sizeof(lora_message_t*));
}

rt_err_t get_lora_downlink_message(lora_message_t **lora_tx_message) {
    if (lora_tx_message == RT_NULL || lora_downlink_mq == RT_NULL) {
        rt_kprintf("get_lora_downlink: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    *lora_tx_message = RT_NULL;
    return rt_mq_recv(lora_downlink_mq, lora_tx_message, sizeof(lora_message_t*), RT_WAITING_NO);
}

// 2. lora uplink message queue
rt_err_t put_lora_uplink_message(lora_message_t *lora_rx_message) {
    if (lora_rx_message == RT_NULL || lora_uplink_mq == RT_NULL) {
        rt_kprintf("put_lora_uplink: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    return rt_mq_send(lora_uplink_mq, &lora_rx_message, sizeof(lora_message_t*));
}

rt_err_t get_lora_uplink_message(lora_message_t **lora_rx_message) {
    if (lora_rx_message == RT_NULL || lora_uplink_mq == RT_NULL) {
        rt_kprintf("get_lora_uplink: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    *lora_rx_message = RT_NULL;
    return rt_mq_recv(lora_uplink_mq, lora_rx_message, sizeof(lora_message_t*), RT_WAITING_NO);
}

// 4. node downlink message queue
rt_err_t put_node_downlink_message(PHYPayload_t **PHYPayload) {
    if (PHYPayload == NULL || *PHYPayload == NULL || node_downlink_mq == RT_NULL) {
        rt_kprintf("put_node_downlink_message: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    return rt_mq_send(node_downlink_mq, PHYPayload, sizeof(PHYPayload_t*));
}

rt_err_t get_node_downlink_message(PHYPayload_t **PHYPayload) {
    if (PHYPayload == NULL ||node_downlink_mq == RT_NULL) {
        rt_kprintf("get_node_downlink_message: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    return rt_mq_recv(node_downlink_mq, PHYPayload, sizeof(PHYPayload_t*), RT_WAITING_NO);  // Standardized to NO
}

// 5. gateway uplink message queue
rt_err_t put_gateway_uplink_message(transport_udp_message_t **gateway_uplink_message) {
    if (gateway_uplink_message == RT_NULL || gateway_uplink_mq == RT_NULL) {
        rt_kprintf("put_gateway_uplink: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    return rt_mq_send(gateway_uplink_mq, gateway_uplink_message, sizeof(transport_udp_message_t*));
}

rt_err_t get_gateway_uplink_message(transport_udp_message_t **gateway_uplink_message) {
    if (gateway_uplink_message == RT_NULL || gateway_uplink_mq == RT_NULL) {
        rt_kprintf("get_gateway_uplink: invalid param/mq not init\n");
        return RT_EINVAL;
    }

    return rt_mq_recv(gateway_uplink_mq, gateway_uplink_message, sizeof(transport_udp_message_t*), RT_WAITING_NO);
}

// 6. gateway downlink message queue
rt_err_t put_gateway_downlink_message(transport_udp_message_t *gateway_downlink_message) {
    if (gateway_downlink_message == RT_NULL || gateway_downlink_mq == RT_NULL) {
        rt_kprintf("put_gateway_downlink_message: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    return rt_mq_send(gateway_downlink_mq, &gateway_downlink_message, sizeof(transport_udp_message_t*));
}

rt_err_t get_gateway_downlink_message(transport_udp_message_t **gateway_downlink_message) {
    if (gateway_downlink_message == RT_NULL || gateway_downlink_mq == RT_NULL) {
        rt_kprintf("get_gateway_downlink_message: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    return rt_mq_recv(gateway_downlink_mq, gateway_downlink_message, sizeof(transport_udp_message_t*), RT_WAITING_NO);
}

// 7. cloud uplink message queue
rt_err_t put_cloud_uplink_message(raw_udp_message_t **raw_udp_message) {
    if (raw_udp_message == RT_NULL || cloud_uplink_mq == RT_NULL) {
        rt_kprintf("put_cloud_uplink_message: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    return rt_mq_send(cloud_uplink_mq, raw_udp_message, sizeof(raw_udp_message_t*));
}

rt_err_t get_cloud_uplink_message(raw_udp_message_t **raw_udp_message) {
    if (raw_udp_message == RT_NULL || cloud_uplink_mq == RT_NULL) {
        rt_kprintf("get_cloud_uplink_message: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    return rt_mq_recv(cloud_uplink_mq, raw_udp_message, sizeof(raw_udp_message_t*), RT_WAITING_NO);
}

// 8. cloud downlink message queue
rt_err_t put_cloud_downlink_message(raw_udp_message_t *raw_udp_message) {
    if (raw_udp_message == RT_NULL || cloud_downlink_mq == RT_NULL) {
        rt_kprintf("put_cloud_downlink_message: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    return rt_mq_send(cloud_downlink_mq, &raw_udp_message, sizeof(raw_udp_message_t*));
}



rt_err_t get_cloud_downlink_message(raw_udp_message_t **raw_udp_message) {
    if (raw_udp_message == RT_NULL || cloud_downlink_mq == RT_NULL) {
        rt_kprintf("get_cloud_downlink_message: invalid param/mq not init\n");
        return RT_EINVAL;
    }
    *raw_udp_message = RT_NULL;
    return rt_mq_recv(cloud_downlink_mq, raw_udp_message, sizeof(raw_udp_message_t*), RT_WAITING_NO);
}


//9. cloud message queue init
int transport_message_queue_init(void) {
    //2.1 create lora_uplink message queue
    if (lora_uplink_mq != RT_NULL) {
        rt_kprintf("lora_uplink_mq already initialized\n");
    } else {
        lora_uplink_mq = rt_mq_create("lora_uplink_mq", sizeof(lora_message_t*), CLOUD_MSG_MAX_LENGTH, RT_IPC_FLAG_FIFO);
        if (lora_uplink_mq == RT_NULL) {
            rt_kprintf("initialize lora_uplink_mq failed\n");
        }
    }

    //2.2 create lora_downlink message queue
    if (lora_downlink_mq != RT_NULL) {
        rt_kprintf("lora_downlink_mq already initialized\n");
    } else {
        lora_downlink_mq = rt_mq_create("lora_downlink_mq", sizeof(lora_message_t*), CLOUD_MSG_MAX_LENGTH, RT_IPC_FLAG_FIFO);
        if (lora_downlink_mq == RT_NULL) {
            rt_kprintf("initialize lora_downlink_mq failed\n");
        }
    }

    //2.4 create node_downlink message queue
    if (node_downlink_mq != RT_NULL) {
        rt_kprintf("node_downlink_mq already initialized\n");
    } else {
        node_downlink_mq = rt_mq_create("node_downlink_mq", sizeof(PHYPayload_t*), CLOUD_MSG_MAX_LENGTH, RT_IPC_FLAG_FIFO);
        if (node_downlink_mq == RT_NULL) {
            rt_kprintf("initialize node_downlink_mq failed\n");
        }
    }

		
		//2.5 create node_downlink message queue
		if (gateway_uplink_mq != RT_NULL) {
        rt_kprintf("gateway_uplink_mq already initialized\n");
    } else {
        gateway_uplink_mq = rt_mq_create("gateway_uplink_mq", sizeof(transport_udp_message_t*), CLOUD_MSG_MAX_LENGTH, RT_IPC_FLAG_FIFO);
        if (gateway_uplink_mq == RT_NULL) {
            rt_kprintf("initialize gateway_uplink_mq failed\n");
        }
    }

    //2.6. create lora_downlink message queue
    if (gateway_downlink_mq != RT_NULL) {
        rt_kprintf("gateway_downlink_mq already initialized\n");
    } else {
        gateway_downlink_mq = rt_mq_create("gateway_downlink_mq", sizeof(transport_udp_message_t*), CLOUD_MSG_MAX_LENGTH, RT_IPC_FLAG_FIFO);
    
			if (gateway_downlink_mq == RT_NULL) {
            rt_kprintf("initialize gateway_downlink_mq failed\n");
        }
    }		
    //2.7. create gateway_uplink message queue
    if (cloud_uplink_mq != RT_NULL) {
        rt_kprintf("cloud_uplink_mq already initialized\n");
    } else {
        cloud_uplink_mq = rt_mq_create("cloud_uplink_mq", sizeof(raw_udp_message_t*), CLOUD_MSG_MAX_LENGTH, RT_IPC_FLAG_FIFO);
        if (cloud_uplink_mq == RT_NULL) {
            rt_kprintf("initialize cloud_uplink_mq failed\n");
        }
    }

    //2.8. create gateway_downlink message queue
    if (cloud_downlink_mq != RT_NULL) {
        rt_kprintf("cloud_downlink_mq already initialized\n");
    } else {
        cloud_downlink_mq = rt_mq_create("cloud_downlink_mq", sizeof(raw_udp_message_t*), CLOUD_MSG_MAX_LENGTH, RT_IPC_FLAG_FIFO);
        if (cloud_downlink_mq == RT_NULL) {
            rt_kprintf("initialize cloud_downlink_mq failed\n");
        }
    }
    return RT_EOK;
}
INIT_COMPONENT_EXPORT(transport_message_queue_init);