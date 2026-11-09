/*
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe:
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
 */
#include "fm25_flash.h"
#include <rtdevice.h>
#include <board.h>
#include "drv_spi.h"

#define FRAM_SPI_NAME      "FM25V01"
#define FRAM_SPI_CS_PIN    GET_PIN(A,4)
              

#define MSB(data)         ((uint8_t)(((data) & 0xff00) >> 8))
#define LSB(data)         ((uint8_t)((data) & 0x00ff))
/*FRAM 寄存器定义*/
#define WRNP      	      0x80u         // 失能块的写保护以及失能WPEN位
#define WREN      	      0x06u         // 写使能
#define WRDI      	      0x04u         // 写禁止
#define RDSR      	      0x05u         // 读状态寄存器
#define WRSR      	      0x01u         // 写状态寄存器
#define READ      	      0x03u         // 读存储器数据
#define FATRD     	      0x0Bu         // 快速读取内存数据
#define WRITE     	      0x02u         // 写存储器数据
#define SLEEP     	      0xB9u         // 进入睡眠模式
#define RDID      	      0x9Fu         // 读设备ID
#define SNR		  		      0xC3u		      // Read S/N.

//#define FRAM_NULL		      0x00
//#define	FRAM_OK			      0xAA


static struct rt_spi_device *spiFram = RT_NULL;

// 写使能
static void FRAM_WREN(void)
{
  uint8_t reg = WREN;
  rt_spi_send(spiFram,&reg, 0x01);
}
// 写禁止
static void FRAM_WRDI(void)
{
  uint8_t reg = WRDI;
  rt_spi_send(spiFram,&reg, 0x01);
}
// 读状态寄存器
static uint8_t FRAM_RDSR(void)
{
  uint8_t stateReg = 0;
  uint8_t reg = RDSR;

  rt_spi_send_then_recv(spiFram, &reg, 0x01,&stateReg, 0x01);
  return stateReg;
}
// 写状态寄存器
static int FRAM_WRSR(uint8_t stateReg)
{
  uint8_t reg[] = {WRSR, stateReg};

  FRAM_WREN();
  rt_spi_send(spiFram,reg, 0x02);
  FRAM_WRDI();
  if(stateReg != FRAM_RDSR())
  {
      return -1;
  }
  return 0;
}

// 读取FRAM的设备信息
static uint16_t FRAM_RDeviceID(void)
{
  uint16_t read_id = 0;
  uint8_t deviceID[9] = {0};
  uint8_t reg = RDID;
  
  rt_spi_send_then_recv(spiFram, &reg, 0x01,deviceID, 0x09);
  
  read_id = (deviceID[7] << 8) | deviceID[8];
  return read_id;
}
// 使FRAM进入睡眠模式
void FRAM_Sleep(void)
{
  uint8_t Reg = SLEEP;
  rt_spi_send(spiFram,&Reg, 0x01);
}

// FRAM 清除·开始地址·大小
int FRAM_Clear(uint16_t Start, uint16_t Size)
{
  uint16_t Num  = Size;
  uint16_t Addr = Start;
  uint8_t  txBuf[4]  = {0};
  if(!Size || (Start + Size) > FRAM_SIZE)
  {
    return -1;
  }
  for(; Addr < Num; Addr++)    // 大概需要300ms的时间
  {
    FRAM_WREN();
    txBuf[0] = WRITE;
    txBuf[1] = MSB(Addr);
    txBuf[2] = LSB(Addr);
    txBuf[3] = 0;
    rt_spi_send(spiFram,txBuf, 4);
    FRAM_WRDI();
  }
  return 0;
}
/*
** FRAM 读取
** 输入：读取的数据·数据标号大写前四位·数量
** 成功：true
*/
flash_status FRAM_Read(uint32_t framaddr, uint8_t *pbuf, uint16_t readnum)
{
  uint8_t txbuf[3] = {0};

  if(framaddr >= FRAM_SIZE)
  {
      return FLASH_ERROR_PARAM;
  }
  txbuf[0] = READ;
  txbuf[1] = MSB(framaddr);
  txbuf[2] = LSB(framaddr);
  rt_spi_send_then_recv(spiFram,txbuf, 0x03,pbuf, readnum);

  return FLASH_OK;
}
/*
** FRAM 快速读取
** 未完成
*/
int FRAM_FastRead(uint32_t framaddr, uint8_t *pbuf, uint16_t readnum)
{
  uint16_t Addr = framaddr;
  uint8_t *pBuf = pbuf;
  uint16_t Num = readnum;
  uint8_t txbuf[4] = {0};

  uint8_t *framTxBuf = txbuf;                	// FRAM发送缓存
  uint8_t *framRxBuf = pBuf;                	// FRAM接收缓存

  if(Addr >= FRAM_SIZE)
  {
      return -1;
  }
  framTxBuf[0] = FATRD;
  framTxBuf[1] = MSB(Addr);
  framTxBuf[2] = LSB(Addr);
  framTxBuf[3] = 0X00; //Dummy byte
  rt_spi_send_then_recv(spiFram,framTxBuf, 0x04,framRxBuf, Num);

  return 0;
}
/*
** FRAM 写入
** 输入：写入的数据·数据标号大写前四位·数量
** 成功：true
*/
flash_status FRAM_Write(uint32_t framaddr, uint8_t *pbuf, uint16_t writenum)
{
  uint8_t *framTxBuf = NULL;
  framTxBuf = (uint8_t *)rt_malloc(writenum + 3); // FRAM发送缓存
  FRAM_WREN();
  framTxBuf[0] = WRITE;
  framTxBuf[1] = MSB(framaddr);
  framTxBuf[2] = LSB(framaddr);
  for(int i = 0; i < writenum; i++)
  {
      framTxBuf[i + 3] = *pbuf++;
  }
  rt_spi_send(spiFram,framTxBuf, 3 + writenum);

  FRAM_WRDI();
  rt_free(framTxBuf);
  return FLASH_OK;
}
/*
** FRAM 初始化
** 成功：true
*/

void FRAM_Init(void)
{
  struct rt_spi_configuration cfg;
  rt_err_t res;
  rt_pin_mode(FRAM_SPI_CS_PIN, PIN_MODE_OUTPUT);
  res = rt_hw_spi_device_attach( "spi1",FRAM_SPI_NAME, GPIOA,GPIO_PIN_4);
  if (res != RT_EOK)
  {
      rt_kprintf("rt_spi_bus_attach_device failed!\r\n");
  }
  spiFram = (struct rt_spi_device *)rt_device_find(FRAM_SPI_NAME);
  if (!spiFram)
  {
    rt_kprintf("spi sample run failed! can't find %s device!\n", FRAM_SPI_NAME);
  }
  
  cfg.data_width = 8;
  cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB; /* SPI Compatible: Mode 0. */
  cfg.max_hz = 2 * 1000000;             /* max 10M */
  
  res = rt_spi_configure(spiFram, &cfg);
  
  if (res != RT_EOK)
  {
      rt_kprintf("rt_spi_configure failed!\r\n");
  }
  res = rt_spi_take_bus(spiFram);
  if (res != RT_EOK)
  {
      rt_kprintf("rt_spi_take_bus failed!\r\n");
  }
  
  res = rt_spi_release_bus(spiFram);
  
  if(res != RT_EOK)
  {
      rt_kprintf("rt_spi_release_bus failed!\r\n");
  }
  if(FRAM_WRSR(WRNP) == -1)
  {
    rt_kprintf("FRAM Init Error\r\n");
  }
  rt_kprintf("FRAM ID %04X\r\n",FRAM_RDeviceID());
  
}



