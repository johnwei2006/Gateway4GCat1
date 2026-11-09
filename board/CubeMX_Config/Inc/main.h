/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PH0_OSC_IN_Pin GPIO_PIN_0
#define PH0_OSC_IN_GPIO_Port GPIOH
#define PH1_OSC_OUT_Pin GPIO_PIN_1
#define PH1_OSC_OUT_GPIO_Port GPIOH
#define EC20_WAKEIN_Pin GPIO_PIN_11
#define EC20_WAKEIN_GPIO_Port GPIOE
#define EC20_USIM_Pin GPIO_PIN_12
#define EC20_USIM_GPIO_Port GPIOE
#define EC20_WAKEOUT_Pin GPIO_PIN_13
#define EC20_WAKEOUT_GPIO_Port GPIOE
#define EC20_RESET_Pin GPIO_PIN_14
#define EC20_RESET_GPIO_Port GPIOE
#define EC20_WDIS_Pin GPIO_PIN_15
#define EC20_WDIS_GPIO_Port GPIOE
#define LoRa_CLK_Pin GPIO_PIN_13
#define LoRa_CLK_GPIO_Port GPIOB
#define LoRa_MOSI_Pin GPIO_PIN_14
#define LoRa_MOSI_GPIO_Port GPIOB
#define LoRa_MISO_Pin GPIO_PIN_15
#define LoRa_MISO_GPIO_Port GPIOB
#define LoRa_RST_Pin GPIO_PIN_8
#define LoRa_RST_GPIO_Port GPIOD
#define LoRa_BUSY_Pin GPIO_PIN_9
#define LoRa_BUSY_GPIO_Port GPIOD
#define LoRa_TXEN_Pin GPIO_PIN_12
#define LoRa_TXEN_GPIO_Port GPIOD
#define EC20_RX_Pin GPIO_PIN_6
#define EC20_RX_GPIO_Port GPIOC
#define EC20_TX_Pin GPIO_PIN_7
#define EC20_TX_GPIO_Port GPIOC
#define DBG_TX_Pin GPIO_PIN_9
#define DBG_TX_GPIO_Port GPIOA
#define DBG_RX_Pin GPIO_PIN_10
#define DBG_RX_GPIO_Port GPIOA
#define RS485_TX_Pin GPIO_PIN_10
#define RS485_TX_GPIO_Port GPIOB
#define RS485_RX_Pin GPIO_PIN_11
#define RS485_RX_GPIO_Port GPIOB
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
