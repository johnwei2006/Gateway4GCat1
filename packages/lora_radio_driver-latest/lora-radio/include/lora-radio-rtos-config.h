/*!
 * \file      lora-radio-rtos-config.h
 *
 * \brief     adapter to different RTOS implementation
 *
 * \copyright SPDX-License-Identifier: Apache-2.0
 *
 * \author    Forest-Rain
 */
 
#ifndef __LORA_RADIO_RTOS_CONFIG_H_
#define __LORA_RADIO_RTOS_CONFIG_H_

#define LORA_RADIO_DRIVER_USING_ON_RTOS_RT_THREAD
//#define LORA_RADIO_DRIVER_USING_ON_RTOS_RT_THREAD_NANO

#ifdef LORA_RADIO_DRIVER_USING_ON_RTOS_RT_THREAD
#elif defined LORA_RADIO_DRIVER_USING_ON_RTOS_RT_THREAD_NANO
#else
    #error "Please Choose your RTOS setup!"
#endif

#ifdef LORA_RADIO_DRIVER_USING_ON_RTOS_RT_THREAD
#include <rtthread.h>
#include <rtdevice.h>
#include "drv_spi.h"

#elif defined LORA_RADIO_DRIVER_USING_ON_RTOS_RT_THREAD_NANO
#include <rtthread.h>

#endif



#endif // end of __LORA_RADIO_RTOS_CONFIG_H_
