/*
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe:
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
*/

#include <string.h>
#include <rtdevice.h>
#include <board.h>
#include "spi.h"
#include <flash_external/w25q128_flash.h>
#include "drv_spi.h"
static struct rt_spi_device *spi_dev_w25q128 = RT_NULL;
/**
 * read FLASH
 * @param  address :start address to read
 * @param  buffer  :data to be read
 * @param  size    :read data size(byte)
 */
flash_status w25q128_flash_read(uint32_t address, uint8_t *buffer, uint32_t size)
{
    if(spi_dev_w25q128 == RT_NULL){
        w25q128_init();
    }

    rt_uint8_t cmd = 1;//"NM25X_ReadData";
    rt_uint8_t addr[3];
    addr[0] = (rt_uint8_t)(address>>16);
    addr[1] = (rt_uint8_t)(address>>8);
    addr[2] = (rt_uint8_t)address;
    struct rt_spi_message *msg1, *msg2, *msg3;
		/*
    msg1->cs_take = 1;
    msg1.cs_release = 0;
    msg1.length = 1;
    msg1.next = &msg2;
    msg1.recv_buf = RT_NULL;
    msg1.send_buf = &cmd;

    msg2.cs_take = 0;
    msg2.cs_release = 0;
    msg2.length = 3;
    msg2.next = &msg3;
    msg2.recv_buf = RT_NULL;
    msg2.send_buf = &addr[0];

    msg3.cs_take = 0;
    msg3.cs_release = 1;
    msg3.length = size;
    msg3.next = RT_NULL;
    msg3.recv_buf = buffer;
    msg3.send_buf = RT_NULL;
		*/
    rt_spi_transfer_message(spi_dev_w25q128, msg1);
}
/**
 * write FLASH
 * @param  address    :start address to write
 * @param  buffer     :data to be written
 * @param  size       :write data size(byte)
 */
flash_status w25q128_flash_write(uint32_t address, uint8_t *buffer, uint32_t size)
{
    if(spi_dev_w25q128 == RT_NULL){
        w25q128_init();
    }
}
flash_status w25q128_init(void){
    rt_uint8_t dummy = 0xFF;

    rt_hw_spi_device_attach("spi3", "spi_dev_w25q128", GPIOB, GPIO_PIN_12);

    spi_dev_w25q128 = (struct rt_spi_device *)rt_device_find("spi_dev_w25q128");
     if (!spi_dev_w25q128)
     {
         rt_kprintf("spi sample run failed! can't find spi20 device!\n" );
     }
    else
     {    /* configure SPI*/
         struct rt_spi_configuration cfg;
         cfg.data_width = 8;
         cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_3 | RT_SPI_MSB;
         cfg.max_hz = 35 * 1000 *1000; //用的主控是stm32F103,所以最大频率不超过35MHz
         rt_spi_configure(spi_dev_w25q128, &cfg);
         rt_spi_send(spi_dev_w25q128, &dummy, 1);//开启传输
     }
     return FLASH_OK;
}
static uint16_t w25q128_read_device_id(void){
     rt_uint8_t cmd = 0x90;
     rt_uint8_t id[5];
     struct rt_spi_message msg1, msg2;
     msg1.cs_take = 1;
     msg1.cs_release = 0;
     msg1.length = 1;
     msg1.next = &msg2;
     msg1.recv_buf = RT_NULL;
     msg1.send_buf = &cmd;

     msg2.cs_take = 0;
     msg2.cs_release = 1;
     msg2.length = 5;
     msg2.next = RT_NULL;
     msg2.recv_buf = id;
     msg2.send_buf = RT_NULL;

     rt_spi_transfer_message(spi_dev_w25q128, &msg1);
     rt_kprintf("w25q128 ID: 0x%x%x.\r\n", id[4], id[3]);
}
