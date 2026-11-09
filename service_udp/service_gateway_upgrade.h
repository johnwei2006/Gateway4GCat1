#ifndef _SERVICE_GATEWAY_UPGRADE_H_
#define _SERVICE_GATEWAY_UPGRADE_H_


#include "abstract_service.h"
#include "../utils/utils.h"
#include "../transport_udp/transport_udp_message.h"
#include "../transport_udp/transport_udp_service_message.h"

#define UPGRADE_THREAD_STACK_SIZE   2048
#define UPGRADE_THREAD_PRIORITY     10
#define UPGRADE_THREAD_TICK         30
#define MAX_HTTP_ERROR_RETRY        5
#define MAX_VERIFY_RETRY            1
#define MAX_MODULE_REBOOT           2
#define DOWNLOAD_URL_BUFFER_SIZE    300

// 升级状态枚举（上报给云平台的状态）
typedef enum {
  UPGRADE_STATUS_DOWNLOAD_START   = 1,
  UPGRADE_STATUS_DOWNLOADING      = 2,
  UPGRADE_STATUS_DOWNLOAD_SUCCESS = 3,
  UPGRADE_STATUS_DOWNLOAD_FAILED  = 4,
  UPGRADE_STATUS_UPDATEING        = 6,
  UPGRADE_STATUS_UPDATE_SUCCESS   = 7,
  UPGRADE_STATUS_UPDATE_FAILED    = 8,
} upgrade_status_t;

typedef enum {
    OTA_TYPE_FULL       = 0x00,
    OTA_TYPE_DIFF       = 0x01,
} upgrade_type;

typedef enum {
    APP_TYPE_BOOTLOADER       = 0x00,
    APP_TYPE_4G_GATEWAY       = 0x01,
    APP_TYPE_INDUSTRY_GATEWAY = 0x02,
    APP_TYPE_DATAWAVE         = 0x03,
    APP_TYPE_IRRIWAVE         = 0x04,
    APP_TYPE_MOTORWAVE        = 0x05,
} app_type;

typedef struct
{
    service_t                   base;
} service_gateway_upgrade_t;

rt_err_t service_execute_gateway_upgrade(void* transport_udp_message, void* business_data);
service_gateway_upgrade_t* service_gateway_upgrade_create(void);

#endif /*_SERVICE_GATEWAY_UPGRADE_H_*/