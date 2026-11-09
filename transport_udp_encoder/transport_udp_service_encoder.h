#ifndef TRANSPORT_UDP_SERVICE_ENCODER_H_
#define TRANSPORT_UDP_SERVICE_ENCODER_H_

#include <stdbool.h>
#include "../transport_udp/transport_udp_message.h"

int transport_udp_service_decode(transport_udp_message_t *udp_message, void  **business_data);
int transport_udp_service_encode(transport_udp_message_t *udp_message);

#endif /*TRANSPORT_UDP_SERVICE_ENCODER_H_*/
