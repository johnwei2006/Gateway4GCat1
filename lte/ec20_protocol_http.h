#ifndef __EC20_PROTOCOL_HTTP_H
#define __EC20_PROTOCOL_HTTP_H
#include "lte_def.h"
#include "at.h"

typedef void (*http_read_callback)(const char *buffer);

void EC20_SetHttpUrl(char **url);
void EC20_SetHttpCommandsFirst(at_response_t resp);
at_command_process_t **EC20_GetHttpCommands(void);
void EC20_SetHttpReadCallback(http_read_callback read_callback);
void EC20_SetHttpGetParam(uint32_t timeout);                       // 简化：只设 timeout
void EC20_SetHttpReadLen(uint32_t read_len);
uint32_t EC20_GetHttpReadOffset(void);
uint32_t EC20_GetHttpReadLen(void);
uint32_t EC20_GetHttpRemainLen(void);
uint32_t EC20_AdvanceHttpOffset(void);

#endif