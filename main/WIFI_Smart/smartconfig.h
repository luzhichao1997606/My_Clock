/*
 * @file: 
 * @Descripttion: 
 * @brief: 
 * @version: 
 * @author: lzc
 * @attention: 注意：
 * @Date: 2020-07-15 18:09:32
 * @LastEditors: lzc
 * @LastEditTime: 2020-07-16 17:17:29
 */ 
#ifndef __SMARTCONFIG_H__
#define __SMARTCONFIG_H__

#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h" 
#include "nvs_flash.h" 
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
TaskHandle_t wifi_CreatedTask;
//创建二值信号量句柄 
SemaphoreHandle_t xSemaphore_WIFI;   
extern SemaphoreHandle_t xSemaphore_WIFI;
void initialise_wifi(void);

#endif