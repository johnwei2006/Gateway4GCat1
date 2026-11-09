#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Automatically generated file; DO NOT EDIT. */
/* RT-Thread Configuration */

/* RT-Thread Kernel */
#define RT_NAME_MAX 20
#define RT_ALIGN_SIZE 4
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 1000
#define RT_USING_OVERFLOW_CHECK
#define IDLE_THREAD_STACK_SIZE 256
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO 4
#define RT_TIMER_THREAD_STACK_SIZE 512

/* kservice optimization */


/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE

/* Memory Management */

#define RT_USING_MEMPOOL
#define RT_USING_SMALL_MEM
#define RT_USING_HEAP
#define RT_USING_THREAD_STACK_CHECK// 启用栈检查
#define RT_USING_OVERFLOW_CHECK     // 启用溢出检测（针对 Cortex-M）

/* Kernel Device Object */

#define RT_USING_DEVICE
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 512
#define RT_CONSOLE_DEVICE_NAME "uart1"
#define RT_VER_NUM 0x40004
#define ARCH_ARM
#define RT_USING_CPU_FFS
#define ARCH_ARM_CORTEX_M
#define ARCH_ARM_CORTEX_M4

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 2048
#define RT_MAIN_THREAD_PRIORITY 10

/* C++ features */


/* Command shell */

#define RT_USING_FINSH
#define FINSH_THREAD_NAME "tshell"
#define FINSH_USING_HISTORY
#define FINSH_HISTORY_LINES 5
#define FINSH_USING_SYMTAB
#define FINSH_USING_DESCRIPTION
#define FINSH_ECHO_DISABLE_DEFAULT
#define FINSH_THREAD_PRIORITY 20
#define FINSH_THREAD_STACK_SIZE 2048
#define FINSH_CMD_SIZE 80
#define FINSH_USING_MSH
#define FINSH_USING_MSH_DEFAULT
#define FINSH_USING_MSH_ONLY
#define FINSH_ARG_MAX 10

/* Device virtual file system */


/* Device Drivers */

#define RT_USING_DEVICE_IPC
#define RT_PIPE_BUFSZ 512
#define RT_USING_SYSTEM_WORKQUEUE
#define RT_SYSTEM_WORKQUEUE_STACKSIZE 2048
#define RT_SYSTEM_WORKQUEUE_PRIORITY 23
#define RT_USING_SERIAL
#define RT_USING_SERIAL_V1
#define RT_SERIAL_USING_DMA
#define BSP_UART1_RX_USING_DMA
#define BSP_UART3_RX_USING_DMA
#define RT_SERIAL_RB_BUFSZ 4096
#define RT_USING_PIN
#define RT_USING_ADC
#define RT_USING_RTC
#define RT_USING_RTC_MANAGER
#define BSP_USING_RTC
#define RT_USING_SPI
#define RT_USING_WDT
#define RT_USING_HWCRYPTO
#define RT_HWCRYPTO_DEFAULT_NAME "hwcryto"
#define RT_HWCRYPTO_IV_MAX_SIZE 16
#define RT_HWCRYPTO_KEYBIT_MAX_SIZE 256

/* Using USB */


/* POSIX layer and C standard library */

#define RT_LIBC_USING_TIME
#define RT_LIBC_DEFAULT_TIMEZONE 8

/* Network */

/* Socket abstraction layer */


/* Network interface device */


/* light weight TCP/IP stack */


/* AT commands */

#define RT_USING_AT
#define AT_USING_CLIENT
#define AT_CLIENT_NUM_MAX 1
#define AT_USING_CLI
#define AT_CMD_MAX_LEN 4096
#define AT_SW_VERSION_NUM 0x10301

/* VBUS(Virtual Software BUS) */


/* Utilities */


/* RT-Thread Utestcases */


/* RT-Thread online packages */

/* IoT - internet of things */


/* Wi-Fi */

/* Marvell WiFi */


/* Wiced WiFi */


/* CYW43012 WiFi */


/* BL808 WiFi */


/* CYW43439 WiFi */


/* IoT Cloud */


/* security packages */


/* language packages */

/* JSON: JavaScript Object Notation, a lightweight data-interchange format */

#define PKG_USING_CJSON
#define PKG_USING_CJSON_LATEST_VERSION

/* XML: Extensible Markup Language */


/* multimedia packages */

/* LVGL: powerful and easy-to-use embedded GUI library */


/* u8g2: a monochrome graphic library */


/* tools packages */


/* system packages */

/* enhanced kernel services */


/* acceleration: Assembly language or algorithmic acceleration packages */


/* CMSIS: ARM Cortex-M Microcontroller Software Interface Standard */


/* Micrium: Micrium software products porting for RT-Thread */


/* peripheral libraries and drivers */

/* HAL & SDK Drivers */

/* STM32 HAL & SDK Drivers */


/* Kendryte SDK */


/* sensors drivers */


/* touch drivers */

#define PKG_USING_LORA_RADIO_DRIVER
#define LORA_RADIO_DRIVER_USING_LORA_CHIP
#define LORA_RADIO_DRIVER_USING_LORA_RADIO_SINGLE_INSTANCE
#define LORA_RADIO0_DEVICE_NAME "lora-radio0"
#define LORA_RADIO0_SPI_BUS_NAME "spi2"
#define LORA_RADIO_DRIVER_USING_LORA_CHIP_SX126X

/* Select Supported LoRa Module [SX126X] */

#define LORA_RADIO_USE_TCXO
#define LORA_RADIO_GPIO_SETUP
#define LORA_RADIO_SPI_SETUP
#define LORA_RADIO_GPIO_SETUP_BY_PIN_NAME
#define LORA_RADIO_NSS_PIN_NAME "PB.12"
#define LORA_RADIO_RESET_PIN_NAME "PD.8"
#define LORA_RADIO_DIO1_PIN_NAME "PD.10"
#define LORA_RADIO_BUSY_PIN_NAME "PD.9"
#define LORA_RADIO_RFSW1_PIN_NAME "PD.12"
#define LORA_RADIO_DRIVER_USING_LORA_MODULE_LSD4RF_2R717N40

/* LoRa Chip SX1268 (SPI module) */


/* Select Supported Target Borad */

#define LORA_RADIO_DRIVER_USING_TRAGET_BOARD_LSD4RF_TEST2002
#define LORA_RADIO_RFSW2_PIN_NAME "PD.13"

/* Select LoRa Radio Driver Example */

#define PKG_USING_LORA_RADIO_DRIVER_LATEST_VERSION

/* AI packages */


/* Signal Processing and Control Algorithm Packages */


/* miscellaneous packages */

/* project laboratory */

/* samples: kernel and components samples */


/* entertainment: terminal games and other interesting software packages */

#define PKG_USING_CRCLIB
#define CRCLIB_USING_CRC8
#define CRC8_USING_CONST_TABLE
#define CRC8_POLY_CUSTOM
#define CRC8_POLY_CUSTOM_STRING 49
#define CRC8_POLY 49
#define CRCLIB_USING_CRC16
#define CRC16_USING_CONST_TABLE
#define CRC16_POLY_A001
#define CRC16_POLY 40961
#define PKG_USING_CRCLIB_V100

/* Arduino libraries */


/* Projects and Demos */


/* Sensors */


/* Display */


/* Timing */


/* Data Processing */


/* Data Storage */

/* Communication */


/* Device Control */


/* Other */


/* Signal IO */


/* Uncategorized */

#define SOC_FAMILY_STM32
#define SOC_SERIES_STM32F4

/* Hardware Drivers Config */

#define SOC_STM32F407VG

/* Onboard Peripheral Drivers */

#define BSP_USING_SPI_LORA

/* On-chip Peripheral Drivers */

#define BSP_USING_GPIO
#define BSP_USING_ONCHIP_RTC
#define BSP_USING_WDT
#define BSP_USING_ON_CHIP_FLASH
#define BSP_USING_ADC1
#define BSP_USING_UART
#define BSP_USING_UART1
#define BSP_USING_UART6
#define BSP_USING_UART3
#define BSP_UART6_RX_BUFSIZE 4096
#define BSP_USING_SPI
#define BSP_USING_SPI1
#define BSP_USING_SPI2
#define BSP_USING_SPI3

/* Board extended module Drivers */


#endif
