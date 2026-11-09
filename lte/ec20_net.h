#ifndef __EC20_NET_H
#define __EC20_NET_H

#include <stdint.h>
#include "lte_def.h"
#include "at.h"

const char *EC20_GetGMI(void);
const char *EC20_GetGMM(void);
const char *EC20_GetGMR(void);
const char *EC20_GetIMEI(void);
const char *EC20_GetICCID(void);
const char *EC20_GetIMSI(void);
const char *EC20_GetOperator(void);
lte_time_t *EC20_GetTime(void);
uint8_t EC20_GetCSQ(void);

void EC20_SetNetCommandsFirst(at_response_t resp);
at_command_process_t **EC20_GetNetCommands(void);

#endif


























