#ifndef __EC20_PROTOCOL_UDP_H
#define __EC20_PROTOCOL_UDP_H   

#include "lte_def.h"
#include "at.h"

typedef void (*udp_recv_callback)(const char *buffer, uint16_t len);

void ec20_set_udp_server(char **server, uint16_t port);
void ec20_set_udp_commands_first(at_response_t resp);
at_command_process_t **ec20_get_udp_connect_commands(void);
at_command_process_t **ec20_get_udp_send_data_commands(void);
at_command_process_t **ec20_get_udp_read_data_commands(void);
at_command_process_t **ec20_get_udp_check_connect_commands(void);
void ec20_set_udp_recv_callback(udp_recv_callback function);
int ec20_prepare_send_data(const char *data, uint16_t len);

static at_command_handle_state_t at_connect_callback(void);
static at_command_handle_state_t at_send_command_callback(void);
static at_command_handle_state_t at_recv_data_callback(void);
static at_command_handle_state_t at_domain_name_resolve_callback(void);
#endif