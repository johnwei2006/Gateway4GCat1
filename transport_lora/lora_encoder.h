#ifndef LORA_ENCODER_H_
#define LORA_ENCODER_H_

#include <stdbool.h>
#include "lora_message.h"

int lora_decode(lora_message_t *rx_message, PHYPayload_t *PHYPayload);
int lora_encode(PHYPayload_t *PHYPayload,      lora_message_t *tx_message);

#endif /*LORA_ENCODER_H_*/
