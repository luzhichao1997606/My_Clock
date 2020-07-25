/*
 * @file:
 * @Descripttion:
 * @brief:
 * @version:
 * @author: lzc
 * @attention: 注意：
 * @Date: 2020-07-18 17:37:44
 * @LastEditors: lzc
 * @LastEditTime: 2020-07-25 10:55:56
 */
#ifndef __HTTPS_OTA_H__
#define __HTTPS_OTA_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_task_wdt.h"
#include "string.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "nvs.h"
#include "nvs_flash.h"

#define CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL "https://47.98.136.66:8070/hello-world.bin"//默认URL

static const char *OTA_TAG = "simple_ota_example";
TaskHandle_t OTA_Handler;
extern TaskHandle_t OTA_Handler;
void simple_ota_example_task(void *pvParameter);
void advanced_ota_example_task(void *pvParameter);
#endif
