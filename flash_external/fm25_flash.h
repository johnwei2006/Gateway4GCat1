#ifndef __FM25_FLASH_H
#define __FM25_FLASH_H

#include <rtthread.h>
#include "abstract_flash_external.h"

/*FM25V01总内存大小*/
#define		FRAM_BASE_ADDR	  		0				        // FM25V01
#define		FRAM_SIZE	  			    0x4000u			    // FM25V01(16K byte)
#define		FRAM_MAXADDR	  		  (FRAM_SIZE - 1)	// FM25V01

void FRAM_Sleep(void);
int FRAM_Clear(uint16_t Start, uint16_t Size);
flash_status FRAM_Read(uint32_t framaddr, uint8_t *pbuf, uint16_t readnum);
int FRAM_FastRead(uint32_t framaddr, uint8_t *pbuf, uint16_t readnum);
flash_status FRAM_Write(uint32_t framaddr, uint8_t *pbuf, uint16_t writenum);
void FRAM_Init(void);

#endif





































