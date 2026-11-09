#include "service_gateway_upgrade.h"
#include "../transport/transport_message_queue.h"
#include <rtthread.h>
#include "../lte/ec20_protocol_http.h"
#include "../lte/lte_app.h"
#include "../utils/cJSON.h"
#include "../utils/utils.h"
#include "../utils/base64.h"
#include "../libraries/HAL_Drivers/drv_flash/drv_flash.h"
#include "../iap/iap.h"

// 静态全局变量：升级线程上下文
static rt_thread_t                     upgrade_thread        = RT_NULL;
static service_data_gateway_upgrade_t *upgrade_data          = RT_NULL;
static uint16_t                        total_package_count   = 0;
static uint16_t                        current_package_index = 0;
static uint32_t                        upgrade_event_id      = 0;
static uint8_t                         http_get_error_count  = 0;
static volatile uint8_t                download_complete     = 0;
static volatile uint8_t                http_request_ok       = 0;
static uint8_t                         verify_retry_count    = 0;
static uint8_t                         module_reboot_count   = 0;

// 前向声明
static void upgrade_process(void* parameter);
static void EC20_HttpReadCallback(const char *buffer);
static void thread_exit_cb(rt_thread_t p_thread);
static void report_gateway_upgrade_status(uint8_t status, uint8_t percent, uint8_t error);
static uint32_t IAP_ProtocolParseData(uint32_t total, const char *buffer, uint8_t type);
static void upgrade_cleanup(void);

/**
* @brief 释放升级线程相关资源
*/
static void upgrade_cleanup(void)
{
    if (upgrade_data != RT_NULL)
    {
        rt_free(upgrade_data);
        upgrade_data = RT_NULL;
    }
    total_package_count   = 0;
    current_package_index = 0;
    http_get_error_count  = 0;
    download_complete     = 0;
    http_request_ok       = 0;
    verify_retry_count    = 0;
    module_reboot_count   = 0;
}

/**
* @brief 执行网关升级服务入口
*
* 接收云平台下发的升级指令，解析升级参数，创建升级线程执行下载流程。
*
* @param transport_udp_message 传输层UDP消息
* @param business_data 业务数据（未使用，从缓存中获取）
* @return rt_err_t RT_EOK 成功，其他值表示失败
*/
rt_err_t service_execute_gateway_upgrade(void* transport_udp_message, void* business_data)
{
    if (transport_udp_message == RT_NULL)
        return RT_EINVAL;

    transport_udp_message_t *transport_udp_message_ptr = (transport_udp_message_t*)transport_udp_message;
    service_data_gateway_upgrade_t *upgrade = RT_NULL;
    get_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id, (void**)&upgrade);
    if (upgrade == RT_NULL)
        return RT_EINVAL;

    // 参数校验
    if (upgrade->firmware_size == 0)
    {
        rt_kprintf("upgrade error: firmware_size is 0\n");
        remove_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id);
        return RT_EINVAL;
    }
    if (upgrade->download_url[0] == '\0')
    {
        rt_kprintf("upgrade error: download_url is empty\n");
        remove_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id);
        return RT_EINVAL;
    }

    // 保存 event_id，用于后续状态上报
    upgrade_event_id = transport_udp_message_ptr->transport_udp_data->event_id;

    // 检查是否已有升级线程在运行
    if (upgrade_thread != RT_NULL)
    {
        rt_kprintf("upgrade warning: upgrade thread already running, skip\n");
        remove_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id);
        return RT_EBUSY;
    }

    // 拷贝升级参数到全局变量（线程安全：此时 upgrade_thread 尚未创建）
    upgrade_data = (service_data_gateway_upgrade_t*)rt_malloc(sizeof(service_data_gateway_upgrade_t));
    if (upgrade_data == RT_NULL)
    {
        rt_kprintf("upgrade error: failed to allocate upgrade_data\n");
        remove_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id);
        return RT_ENOMEM;
    }
    rt_memset(upgrade_data, 0, sizeof(service_data_gateway_upgrade_t));

    upgrade_data->app_type      = upgrade->app_type;
    upgrade_data->upgrade_type  = upgrade->upgrade_type;
    upgrade_data->firmware_size = upgrade->firmware_size;
    rt_strncpy(upgrade_data->upgrade_id,  upgrade->upgrade_id,  sizeof(upgrade_data->upgrade_id) - 1);
    rt_strncpy(upgrade_data->download_url, upgrade->download_url, sizeof(upgrade_data->download_url) - 1);
    rt_strncpy(upgrade_data->version,      upgrade->version,      sizeof(upgrade_data->version) - 1);
    rt_strncpy(upgrade_data->md5,          upgrade->md5,          sizeof(upgrade_data->md5) - 1);

    // 释放业务数据缓存
    remove_business_data_cache(transport_udp_message_ptr->transport_udp_data->event_id);

    // 上报升级开始状态
    report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOAD_START, 0, 0);

    // 创建升级线程
    upgrade_thread = rt_thread_create("upgrade_th",
                                       upgrade_process,
                                       RT_NULL,
                                       UPGRADE_THREAD_STACK_SIZE,
                                       UPGRADE_THREAD_PRIORITY,
                                       UPGRADE_THREAD_TICK);
    if (upgrade_thread != RT_NULL)
    {
        upgrade_thread->cleanup = thread_exit_cb;
        rt_thread_startup(upgrade_thread);
        rt_kprintf("upgrade: thread created, start downloading...\n");
    }
    else
    {
        rt_kprintf("upgrade error: failed to create upgrade_thread\n");
        report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOAD_FAILED, 0, 0xFF);
        upgrade_cleanup();
        return RT_ERROR;
    }

    return RT_EOK;
}

/**
* @brief 升级线程主函数
*
* 完整升级流程：
*   1. 循环下载固件分片到备份区
*   2. 下载完成后校验 CRC/MD5
*   3. 写入升级标志到 Flash
*   4. 重启设备，由 Bootloader 接管升级
*
* @param parameter 线程参数（未使用）
*/
static void upgrade_process(void* parameter)
{
    uint32_t backup_addr;
    uint32_t crc;

    rt_kprintf("=== upgrade_process started ===\n");

    if (upgrade_data == RT_NULL)
    {
        rt_kprintf("upgrade error: upgrade_data is NULL\n");
        return;
    }

    rt_kprintf("upgrade_id   : %s\n", upgrade_data->upgrade_id);
    rt_kprintf("upgrade_type : %d\n", upgrade_data->upgrade_type);
    rt_kprintf("app_type     : %d\n", upgrade_data->app_type);
    rt_kprintf("version      : %s\n", upgrade_data->version);
    rt_kprintf("firmware_size: %u\n", (unsigned int)upgrade_data->firmware_size);
    rt_kprintf("download_url : %s\n", upgrade_data->download_url);
    rt_kprintf("md5          : %s\n", upgrade_data->md5);

    // 计算总分片数
    total_package_count = upgrade_data->firmware_size / IAP_APP_PKG_SIZE
                        + (upgrade_data->firmware_size % IAP_APP_PKG_SIZE != 0 ? 1 : 0);

    // 确定备份区地址
    backup_addr = (upgrade_data->app_type == APP_TYPE_INDUSTRY_GATEWAY)
                  ? BACKUP_APP_BASE_ADDRESS : LORA_APP_BASE_ADDRESS;
    rt_kprintf("backup addr: 0x%08X, total packages: %u\n",
               (unsigned int)backup_addr, (unsigned int)total_package_count);

    verify_retry_count = 0;

    do
    {
        if (verify_retry_count > 0)
        {
            rt_kprintf("=== verify retry %u/%u, re-download all packages ===\n",
                       (unsigned int)verify_retry_count,
                       (unsigned int)MAX_VERIFY_RETRY);
            report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOADING, 0, 0);
        }
        else
        {
            report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOADING, 0, 0);
        }

        // 重置下载状态
        current_package_index = 0;
        http_get_error_count  = 0;
        download_complete     = 0;
        http_request_ok       = 0;
        module_reboot_count   = 0;

        // ========== 阶段1：分片下载固件到备份区 ==========
        while (!download_complete)
        {
            rt_kprintf("downloading: pack %u / %u\n",
                       (unsigned int)current_package_index,
                       (unsigned int)total_package_count);

            // 当前分片连续错误次数超限，尝试重启模组恢复连接
            if (http_get_error_count > MAX_HTTP_ERROR_RETRY)
            {
                if (module_reboot_count < MAX_MODULE_REBOOT)
                {
                    module_reboot_count++;
                    uint8_t progress = (uint8_t)((uint32_t)current_package_index * 100 / total_package_count);
                    rt_kprintf("pack %u failed %u times, rebooting LTE module (attempt %u/%u)...\n",
                               (unsigned int)current_package_index,
                               (unsigned int)http_get_error_count,
                               (unsigned int)module_reboot_count,
                               (unsigned int)MAX_MODULE_REBOOT);
                    report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOADING, progress, 0);

                    ltePort.LTE_Reboot();

                    rt_kprintf("LTE module reboot complete, retrying pack %u...\n",
                               (unsigned int)current_package_index);

                    // 重置当前分片错误计数，准备重试
                    http_get_error_count = 0;
                    http_request_ok      = 0;
                    continue;
                }
                else
                {
                    uint8_t progress = (uint8_t)((uint32_t)current_package_index * 100 / total_package_count);
                    rt_kprintf("upgrade failed: too many http errors (%u) on pack %u after %u module reboots\n",
                               (unsigned int)http_get_error_count,
                               (unsigned int)current_package_index,
                               (unsigned int)MAX_MODULE_REBOOT);
                    report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOAD_FAILED,
                                                  progress, http_get_error_count);
                    return;
                }
            }

            // 构造分片下载 URL
            char *http_download_url = (char *)rt_malloc(DOWNLOAD_URL_BUFFER_SIZE);
            if (http_download_url == RT_NULL)
            {
                rt_kprintf("upgrade failed: malloc for http url failed\n");
                uint8_t progress = (uint8_t)((uint32_t)current_package_index * 100 / total_package_count);
                report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOAD_FAILED, progress, 0xFF);
                return;
            }
            rt_snprintf(http_download_url, DOWNLOAD_URL_BUFFER_SIZE,
                        "%s?packSize=%u&pack=%u",
                        upgrade_data->download_url,
                        (unsigned int)IAP_APP_PKG_SIZE,
                        (unsigned int)current_package_index);

            // 配置 HTTP GET 请求
            http_request_ok = 0;
            EC20_SetHttpReadLen(IAP_APP_PKG_SIZE);
            EC20_SetHttpUrl(&http_download_url);
            EC20_SetHttpReadCallback(EC20_HttpReadCallback);

            // 执行 HTTP 请求
            lte_command_proccess(ltePort.httpCommands.LTE_GetCommands());
            rt_free(http_download_url);

            if (download_complete)
                break;

            // 仅在当前分片请求成功时推进 HTTP 偏移量，失败则重试同一分片
            if (http_request_ok)
            {
                EC20_AdvanceHttpOffset();
            }
            rt_thread_mdelay(10);
        }

        // ========== 阶段2：固件完整性校验 ==========
        if (!download_complete)
            break;

        rt_kprintf("=== download complete, start verification ===\n");
        report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOAD_SUCCESS, 100, 0);

        if (upgrade_data->app_type != APP_TYPE_INDUSTRY_GATEWAY)
            break;

        // 上报正在更新状态
        report_gateway_upgrade_status(UPGRADE_STATUS_UPDATEING, 0, 0);

        // 2.1 计算固件 CRC
        crc = IAP_CalcFirmwareCRC(BACKUP_APP_BASE_ADDRESS,
                                   upgrade_data->firmware_size);
        rt_kprintf("Firmware CRC: 0x%08X\n", (unsigned int)crc);

        // 2.2 计算固件 MD5 并校验
        if (upgrade_data->md5[0] != '\0')
        {
            char md5_hex[33];
            rt_memset(md5_hex, 0, sizeof(md5_hex));

            IAP_CalcFirmwareMD5(BACKUP_APP_BASE_ADDRESS,
                                upgrade_data->firmware_size, md5_hex);
            rt_kprintf("Firmware MD5 : %s\n", md5_hex);
            rt_kprintf("Expected MD5 : %s\n", upgrade_data->md5);

            if (rt_strcasecmp(md5_hex, upgrade_data->md5) != 0)
            {
                rt_kprintf("MD5 mismatch!\n");
                verify_retry_count++;

                if (verify_retry_count <= MAX_VERIFY_RETRY)
                {
                    rt_kprintf("MD5 verify failed, will retry download (attempt %u/%u)\n",
                               (unsigned int)verify_retry_count,
                               (unsigned int)MAX_VERIFY_RETRY);
                    continue;
                }
                else
                {
                    rt_kprintf("MD5 verify failed after %u retries, upgrade aborted.\n",
                               (unsigned int)MAX_VERIFY_RETRY);
                    report_gateway_upgrade_status(UPGRADE_STATUS_UPDATE_FAILED, 0, 0xFF);
                    return;
                }
            }
            rt_kprintf("MD5 verification passed.\n");
        }

        break;  // 校验通过，退出重试循环

    } while (verify_retry_count <= MAX_VERIFY_RETRY);

    if (!download_complete)
        return;

    // ========== 阶段3：写入升级标志 ==========
    upgrade_flag_t flag;
    rt_memset(&flag, 0xFF, sizeof(flag));
    flag.upgrade_status = UPGRADE_STATUS_DOWNLOADED;
    flag.app_type       = upgrade_data->app_type;
    flag.upgrade_type   = upgrade_data->upgrade_type;
    flag.firmware_size  = upgrade_data->firmware_size;
    flag.firmware_crc   = crc;

    if (upgrade_flag_write(&flag) != 0)
    {
        rt_kprintf("upgrade failed: write upgrade flag error\n");
        report_gateway_upgrade_status(UPGRADE_STATUS_UPDATE_FAILED, 0, 0xFE);
        return;
    }
    rt_kprintf("upgrade flag written, status=DOWNLOADED\n");

    // 上报升级成功（更新完成，等待重启）
    report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOAD_SUCCESS, 100, 0);

    // ========== 阶段4：重启设备 ==========
    rt_kprintf("Rebooting to apply upgrade...\n");
    rt_thread_mdelay(500);

    HAL_NVIC_SystemReset();
}

/**
* @brief HTTP 下载回调函数
*
* 每收到一个固件分片后由 HTTP 模块回调，解析 JSON 并写入 Flash。
* 根据解析结果更新下载进度或错误计数。
*
* @param buffer HTTP 响应数据缓冲区
*/
static void EC20_HttpReadCallback(const char *buffer)
{
    if (upgrade_data == RT_NULL)
        return;

    uint32_t parsed_index = IAP_ProtocolParseData(total_package_count, buffer,
                                                   upgrade_data->app_type);

    if (parsed_index > 0)
    {
        // 当前分片下载成功：重置错误计数，标记请求成功
        http_get_error_count = 0;
        http_request_ok      = 1;

        current_package_index = parsed_index;
        uint8_t progress = (uint8_t)((uint32_t)current_package_index * 100 / total_package_count);

        if (current_package_index >= total_package_count)
        {
            download_complete = 1;
            rt_kprintf("all packages download complete!\n");
            report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOAD_SUCCESS, 100, 0);
            rt_thread_mdelay(100);
        }
        else
        {
            report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOADING, progress, 0);
            rt_kprintf("download progress: %u%%, pack: %u/%u\n",
                       (unsigned int)progress,
                       (unsigned int)current_package_index,
                       (unsigned int)total_package_count);
            rt_thread_mdelay(100);
        }
    }
    else
    {
        // 当前分片解析失败，标记请求失败，累加错误计数
        http_request_ok = 0;
        http_get_error_count++;
        rt_kprintf("http parse error on pack %u, retry count: %u/%u\n",
                   (unsigned int)current_package_index,
                   (unsigned int)http_get_error_count,
                   (unsigned int)MAX_HTTP_ERROR_RETRY);

        if (http_get_error_count > MAX_HTTP_ERROR_RETRY)
        {
            download_complete = 1;
            uint8_t progress = (uint8_t)((uint32_t)current_package_index * 100 / total_package_count);
            report_gateway_upgrade_status(UPGRADE_STATUS_DOWNLOAD_FAILED,
                                          progress, http_get_error_count);
        }
    }
}

/**
* @brief 上报网关升级状态到云平台
*
* 构造并发送升级状态上报消息。
*
* @param status  升级状态（参考 upgrade_status_t）
* @param percent 下载/升级进度百分比 (0-100)
* @param error   错误码（0 表示无错误）
*/
static void report_gateway_upgrade_status(uint8_t status, uint8_t percent, uint8_t error)
{
    if (upgrade_data == RT_NULL)
    {
        rt_kprintf("report_gateway_upgrade_status: upgrade_data is NULL, skip\n");
        return;
    }

    transport_udp_message_t* gateway_uplink_message = (transport_udp_message_t*)rt_malloc(sizeof(transport_udp_message_t));
    if (gateway_uplink_message == RT_NULL)
    {
        rt_kprintf("report upgrade status: malloc gateway_uplink_message failed\n");
        return;
    }
    rt_memset(gateway_uplink_message, 0, sizeof(transport_udp_message_t));

    gateway_uplink_message->transport_udp_header = (transport_udp_header_t*)rt_malloc(sizeof(transport_udp_header_t));
    gateway_uplink_message->transport_udp_data   = (transport_udp_data_t*)rt_malloc(sizeof(transport_udp_data_t));
    gateway_uplink_message->service_data         = (service_data_t*)rt_malloc(sizeof(service_data_t));

    if (gateway_uplink_message->transport_udp_header == RT_NULL ||
        gateway_uplink_message->transport_udp_data   == RT_NULL ||
        gateway_uplink_message->service_data         == RT_NULL)
    {
        rt_kprintf("report upgrade status: malloc sub-struct failed\n");
        if (gateway_uplink_message->transport_udp_header != RT_NULL)
            rt_free(gateway_uplink_message->transport_udp_header);
        if (gateway_uplink_message->transport_udp_data != RT_NULL)
            rt_free(gateway_uplink_message->transport_udp_data);
        if (gateway_uplink_message->service_data != RT_NULL)
            rt_free(gateway_uplink_message->service_data);
        rt_free(gateway_uplink_message);
        return;
    }

    rt_memset(gateway_uplink_message->transport_udp_header, 0, sizeof(transport_udp_header_t));
    rt_memset(gateway_uplink_message->transport_udp_data,   0, sizeof(transport_udp_data_t));
    rt_memset(gateway_uplink_message->service_data,         0, sizeof(service_data_t));

    gateway_uplink_message->transport_udp_data->event_id       = upgrade_event_id;
    gateway_uplink_message->transport_udp_data->service_type   = SERVICE_TYPE_GATEWAY_UPGRADE_STATUS;
    gateway_uplink_message->transport_udp_data->service_version = SERVICE_VERSION_1;
    gateway_uplink_message->service_data->business_data_payload = RT_NULL;

    service_data_gateway_upgrade_status_t *upgrade_status =
        (service_data_gateway_upgrade_status_t*)rt_malloc(sizeof(service_data_gateway_upgrade_status_t));
    if (upgrade_status == RT_NULL)
    {
        rt_kprintf("report upgrade status: malloc upgrade_status failed\n");
        rt_free(gateway_uplink_message->transport_udp_header);
        rt_free(gateway_uplink_message->transport_udp_data);
        rt_free(gateway_uplink_message->service_data);
        rt_free(gateway_uplink_message);
        return;
    }

    rt_memset(upgrade_status, 0, sizeof(service_data_gateway_upgrade_status_t));
    rt_strncpy(upgrade_status->upgrade_id, upgrade_data->upgrade_id,
               sizeof(upgrade_status->upgrade_id) - 1);
    upgrade_status->app_type = upgrade_data->app_type;
    upgrade_status->status   = status;
    upgrade_status->percent  = percent;
    upgrade_status->error    = error;

    put_business_data_cache(gateway_uplink_message->transport_udp_data->event_id, upgrade_status);
    rt_kprintf("report upgrade status: status=%u, percent=%u, error=%u\n",
               (unsigned int)status, (unsigned int)percent, (unsigned int)error);

    put_gateway_uplink_message(&gateway_uplink_message);
}

/**
* @brief 升级线程退出回调
*
* 在线程被删除时自动调用，释放线程相关资源。
*
* @param p_thread 被删除的线程句柄
*/
static void thread_exit_cb(rt_thread_t p_thread)
{
    if (p_thread == upgrade_thread)
    {
        rt_kprintf("upgrade_thread exit, cleanup resources\n");
        upgrade_thread = RT_NULL;
        upgrade_cleanup();
    }
}

/**
* @brief 解析 HTTP 响应的 JSON 固件分片数据
*
* JSON 格式：{"pack": N, "size": M, "data": "base64_encoded"}
* 将 base64 解码后的二进制数据写入 Flash 备份区对应偏移位置。
*
* @param total  总分片数（保留参数，用于日志）
* @param buffer HTTP 响应 JSON 字符串
* @param type   应用类型，决定写入哪个备份区
* @return uint32_t 成功返回下一个分片索引 (pack+1)，失败返回 0
*/
static uint32_t IAP_ProtocolParseData(uint32_t total, const char *buffer, uint8_t type)
{
    cJSON *root;
    cJSON *json;
    uint8_t pkgBuffer[IAP_APP_PKG_SIZE];
    uint32_t pack, size;
    uint32_t address;
    int result;

    if (buffer == RT_NULL)
        return 0;

    root = cJSON_Parse(buffer);
    if (root == RT_NULL)
        return 0;

    json = cJSON_GetObjectItem(root, "pack");
    if (json == RT_NULL) { cJSON_Delete(root); return 0; }
    pack = (uint32_t)json->valueint;

    json = cJSON_GetObjectItem(root, "size");
    if (json == RT_NULL) { cJSON_Delete(root); return 0; }
    size = (uint32_t)json->valueint;

    if (size > IAP_APP_PKG_SIZE)
    {
        rt_kprintf("IAP_ProtocolParseData: size %u exceeds max %u\n",
                   (unsigned int)size, (unsigned int)IAP_APP_PKG_SIZE);
        cJSON_Delete(root);
        return 0;
    }

    json = cJSON_GetObjectItem(root, "data");
    if (json == RT_NULL || json->valuestring == RT_NULL)
    {
        cJSON_Delete(root);
        return 0;
    }

    result = b64_to_bin(json->valuestring, rt_strlen(json->valuestring),
                        pkgBuffer, sizeof(pkgBuffer));
    cJSON_Delete(root);

    if (result <= 0)
    {
        rt_kprintf("IAP_ProtocolParseData: base64 decode failed\n");
        return 0;
    }

    address = (type == APP_TYPE_4G_GATEWAY) ? BACKUP_APP_BASE_ADDRESS : LORA_APP_BASE_ADDRESS;
    address += pack * IAP_APP_PKG_SIZE;

    if (stm32_flash_write(address, pkgBuffer, result) != 0)
    {
        rt_kprintf("IAP_ProtocolParseData: flash write failed at 0x%08X\n", (unsigned int)address);
        return 0;
    }

    return pack + 1;
}

/**
* @brief 创建网关升级服务实例
*
* @return service_gateway_upgrade_t* 服务实例指针，失败返回 NULL
*/
service_gateway_upgrade_t* service_gateway_upgrade_create()
{
    service_gateway_upgrade_t* service = (service_gateway_upgrade_t*)rt_malloc(sizeof(service_gateway_upgrade_t));

    if (service == RT_NULL)
    {
        rt_kprintf("Error: Failed to allocate memory for service_gateway_upgrade_t\n");
        return RT_NULL;
    }
    rt_memset(service, 0, sizeof(service_gateway_upgrade_t));
    service->base.service_type    = SERVICE_TYPE_GATEWAY_UPGRADE;
    service->base.service_execute = service_execute_gateway_upgrade;

    return service;
}