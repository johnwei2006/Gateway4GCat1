
#ifndef BYTE_UTILS_H_
#define BYTE_UTILS_H_

#include<string.h>
#include<stdio.h>
#include <stdint.h>
#include <rtthread.h>

void array2hex(uint8_t* in, int length, uint8_t* out);
rt_err_t hex_str_to_bytes(uint8_t *hex_str, uint8_t *byte_buf, uint16_t *buf_len);
#endif /*BYTE_UTILS_H_*/
