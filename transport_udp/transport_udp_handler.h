#ifndef TRANSPORT_UDP_HANDLER_H_
#define TRANSPORT_UDP_HANDLER_H_

#include "../transport/transport_message_queue.h"

#define THREAD_STACK_SIZE 1024
#define THREAD_PRIORITY  8
#define THREAD_TIMESLICE 40
#define UPLINK_MESSAGE_PERIOD_RESEND_INTERVAL 60*1000

extern rt_thread_t cloud_uplink_thread;
extern rt_thread_t cloud_downlink_thread;
extern rt_thread_t uplink_message_period_resend_thread;

int transport_udp_handler_init(void);

#endif /*TRANSPORT_UDP_HANDLER_H_*/