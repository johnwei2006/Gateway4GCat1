#ifndef LORA_SERVICE_ENCODER_H_
#define LORA_SERVICE_ENCODER_H_

#include "lora_message.h"

void lora_service_encoder_init(void);

// convert PHYPayload->mac_payload->service_data to PHYPayload.mac_payload->frm_payload
int lora_service_encode(PHYPayload_t *PHYPayload);

// convert PHYPayload->mac_payload->frm_payload to PHYPayload.mac_payload->service_data
int lora_service_decode(PHYPayload_t *PHYPayload);

#endif /*LORA_SERVICE_ENCODER_H_*/
