/*
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe:
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
 * 2021-6-28     magnin          the first version
 */
#include "ec20_hal.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "lte_def.h"
static at_response_t atResp;
//初始化EC20 HAL层
static void EC20_HalInit(void)
{
  int result;
  //初始化IO口
 // rt_pin_mode(EC20_WAKEIN_PIN, PIN_MODE_INPUT);
  rt_pin_mode(EC20_USIM_PIN, PIN_MODE_INPUT);
//  rt_pin_mode(EC20_WAKEOUT_PIN, PIN_MODE_INPUT);
  rt_pin_mode(EC20_RESET_PIN, PIN_MODE_OUTPUT);
//  rt_pin_mode(EC20_WDISABLE_PIN, PIN_MODE_OUTPUT);
  rt_pin_mode(EC20_POWER_PIN, PIN_MODE_OUTPUT);
  //初始化AT Client
	rt_thread_mdelay(100);
	  rt_pin_write(EC20_POWER_PIN, PIN_LOW);

    rt_pin_write(EC20_RESET_PIN, PIN_HIGH);
    
    
  result = at_client_init(AT_CLIENT_NAME,AT_BUFFER_SIZE);
  if(result != 0)
  {
    LOG_EC20("AT client init error,error code %d",result);
    return ;
  }
  at_delete_resp(atResp);
  atResp = at_create_resp(DEFAULT_AT_RESPONE_BUFFER_SIZE, DEFAULT_AT_RESPONE_LINE_NUMBER, DEFAULT_AT_RESPONE_TIMEOUT);
  if(atResp == RT_NULL)
  {
    LOG_EC20("AT client creat resp error ");
    return ;
  }
  LOG_EC20("EC20 init done!");
}

void power_on_sequence(void)
{
    // 1. 拉低 PWRKEY
    rt_pin_write(EC20_POWER_PIN, PIN_HIGH);
    rt_kprintf("PWRKEY is pulled low, triggering the module to power on....\n");
    // 2. Wait for the internal power supply VDD_EXT to be established (50ms is sufficient, which is more reliable than 10ms to avoid unstable power supply)
    rt_thread_mdelay(35);
  
    rt_pin_write(EC20_RESET_PIN, PIN_LOW);
	  rt_kprintf("RESET_N is pulled low, and the module enters the reset state...\n");
    rt_thread_mdelay(1000);
	
	    // Release reset (RESET_N pulled high, ending the reset state)
   // rt_pin_write(EC20_RESET_PIN, PIN_LOW);
	 // rt_kprintf("RESET_N is pulled low, and the module enters the reset state...\n");
   
    
    // 6. 拉高 PWRKEY
    rt_pin_write(EC20_POWER_PIN, PIN_LOW);
	  rt_kprintf("PWRKEY is pulled high, and the power-on reset process is completed\n");
}








////EC20 上电
//static void EC20_PowerOn(void )
//{
//  LOG_EC20("EC20 power on......");
//  rt_pin_write(EC20_POWER_PIN, PIN_HIGH);
//  rt_thread_mdelay(1500);
//  rt_pin_write(EC20_POWER_PIN, PIN_LOW);
//  rt_thread_mdelay(2000);
//}
////硬件复位EC20
//static void EC20_HardReset(void)
//{
//  LOG_EC20("EC20 Reset(about 20s)......");
//  /*通过RST引脚进行复位*/
//  rt_pin_write(EC20_RESET_PIN, PIN_LOW);
//  rt_thread_mdelay(500);
//  rt_pin_write(EC20_RESET_PIN, PIN_HIGH);
//  rt_thread_mdelay(300);
//}
//EC20唤醒
//void EC20_Wakeup(void)
//{
//    // 实现唤醒逻辑（如拉高唤醒引脚）
//    rt_pin_write(EC20_WAKEIN_PIN, PIN_HIGH);
//    LOG_EC20("EC20 wakeup");
//}
//EC20睡眠
//void EC20_Sleep(void)
//{
//    // 实现睡眠逻辑（如拉低唤醒引脚）
//    rt_pin_write(EC20_WAKEIN_PIN, PIN_LOW);
//    LOG_EC20("EC20 enter sleep");
//}
////检测USIM插入状态
ec20_usim_state_t EC20_UsimInsertState(void)
{
  if(rt_pin_read(EC20_USIM_PIN))
    return EC20_USIM_INSERTED;
  else
    return EC20_USIM_UNINSERTED;
}
at_response_t EC20_GetAtResp(void)
{
  return atResp;
}
//EC20重启
void EC20_Reboot(void)
{
//  EC20_PowerOn();
	power_on_sequence();
 // EC20_HardReset();
  rt_thread_mdelay(15000);    
} 
//EC20初始化
at_response_t EC20_Boot(void)
{
  EC20_HalInit();
  EC20_Reboot();
  return atResp;
}
/*************************************************end file*******************************************************/
