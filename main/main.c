/*
 * @file:
 * @Descripttion:
 * @brief:
 * @version:
 * @author: lzc
 * @attention: 注意：
 * @Date: 2020-07-18 13:39:48
 * @LastEditors: lzc
 * @LastEditTime: 2020-07-28 17:05:24
 */
/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
///< RTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
///< ESP
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
///< nvs&SmartConfig
#include "nvs_flash.h"
#include "smartconfig.h"
///< ota
#include "https_ota.h"
#define CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH 0
///< 全局变量
static const char *SNTP_Tag = "SNTP";
///< 函数定义以及声明
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(SNTP_Tag, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
    ESP_LOGI(SNTP_Tag, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}
static void obtain_time(void)
{
    initialize_sntp();
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
    {
        ESP_LOGI(SNTP_Tag, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}
static void Get_ChipInfo()
{
    printf("Hello world!\n");
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    printf("silicon revision %d, ", chip_info.revision);
    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
}


void app_main()
{
    time_t now;
    bool Falg = false; // @suppress("Type cannot be resolved")
    struct tm timeinfo;
    char strftime_buf[64];
    //WIFI初始化
    xSemaphore_WIFI = xSemaphoreCreateBinary(); // @suppress("Type cannot be resolved")
    Get_ChipInfo();
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    printf("Ver is 1.0.0 \r\n");
    ESP_ERROR_CHECK(err);
    initialise_wifi();
    while (xSemaphoreTake(xSemaphore_WIFI, (TickType_t) 10) != pdTRUE); // @suppress("Symbol is not resolved")
    Falg = true; // @suppress("Symbol is not resolved")
    if (Falg)
    {
        //time(&now);
        //localtime_r(&now, &timeinfo);
        ////是否设置了时间？如果不是，则tm_year将为（1970-1900）。
        //if (timeinfo.tm_year < (2016 - 1900))
        //{
        //    ESP_LOGI(SNTP_Tag, "时间尚未设置,通过NTP获得时间");
        //    obtain_time();
        //    // 用当前时间更新'now'变量
        //    time(&now);
        //}
        //// 将时区设置为中国标准时间
        //setenv("TZ", "CST-8", 1);
        //tzset();
        //localtime_r(&now, &timeinfo);
        //strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        //ESP_LOGI(SNTP_Tag, "The current date/time in Shanghai is: %s", strftime_buf);
        //if (sntp_get_sync_mode() == SNTP_SYNC_MODE_SMOOTH)
        //{
        //    struct timeval outdelta;
        //    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS)
        //    {
        //        adjtime(NULL, &outdelta);
        //        ESP_LOGI(SNTP_Tag, "Waiting for adjusting time ... outdelta = %li sec: %li ms: %li us",
        //                 outdelta.tv_sec,
        //                 outdelta.tv_usec / 1000,
        //                 outdelta.tv_usec % 1000);
        //        vTaskDelay(2000 / portTICK_PERIOD_MS);
        //    }
        //}
        ////xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, &OTA_Handler);
        ////xTaskCreate(&advanced_ota_example_task, "advanced_ota_example_task", 1024 * 8, NULL, 5, &OTA_Handler);
        xTaskCreate(&ota_example_task, "ota_example_task", 8192, NULL, 5, &OTA_Handler);
    }
}
