/*
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe: MD5
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
 * 2024-8-3      Kebai          the first version
 */

#ifndef __MD5_H
#define __MD5_H

#include <rtthread.h>

typedef struct
{
    rt_uint32_t total[2];
    rt_uint32_t state[4];
    rt_uint8_t  buffer[64];
} md5_context_t;

void md5_starts(md5_context_t *ctx);
void md5_update(md5_context_t *ctx, const rt_uint8_t *input, rt_uint32_t ilen);
void md5_finish(md5_context_t *ctx, rt_uint8_t output[16]);

#endif