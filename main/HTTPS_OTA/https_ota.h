/*
 * @file:
 * @Descripttion:
 * @brief:
 * @version:
 * @author: lzc
 * @attention: 注意：
 * @Date: 2020-07-18 17:37:44
 * @LastEditors: lzc
 * @LastEditTime: 2020-07-29 18:20:58
 */
#ifndef __HTTPS_OTA_H__
#define __HTTPS_OTA_H__
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_flash_partitions.h"
#include "esp_partition.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_task_wdt.h"
#include "driver/gpio.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "errno.h"

//LWIP_http
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL "https://47.98.136.66:8070/hello-world.bin"//默认URL

static const char *OTA_TAG = "OTA_Task";
TaskHandle_t OTA_Handler;
extern TaskHandle_t OTA_Handler;
void Check_SHA256(void);
//https
void simple_ota_example_task(void *pvParameter);
void advanced_ota_example_task(void *pvParameter);
//http
void ota_example_task(void *pvParameter);
// 使用方法：
///< xTaskCreate(&ota_example_task, "ota_example_task", 8192, NULL, 5, &OTA_Handler);
#endif
