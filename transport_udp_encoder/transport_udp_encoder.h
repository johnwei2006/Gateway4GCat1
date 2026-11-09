#ifndef TRANSPORT_UDP_ENCODER_H_
#define TRANSPORT_UDP_ENCODER_H_

#include <stdbool.h>
#include "../transport_udp/transport_udp_message.h"

int transport_udp_decode(raw_udp_message_t *raw_udp_message, transport_udp_message_t *udp_message);
int transport_udp_encode(transport_udp_message_t *udp_message, raw_udp_message_t *raw_udp_message);

#endif /*TRANSPORT_UDP_ENCODER_H_*/
