/*
 * @file:
 * @Descripttion:
 * @brief:
 * @version:
 * @author: lzc
 * @attention: 注意：
 * @Date: 2020-07-18 17:37:34
 * @LastEditors: lzc
 * @LastEditTime: 2020-07-25 13:23:38
 */
#include "https_ota.h"

#define OTA_URL_SIZE 256
#define CONFIG_EXAMPLE_OTA_RECV_TIMEOUT     600*1000 //10分钟超时时间
#define OneSec_RTOS                         1000     //1秒计时

char OTA_Adder_IP[] = "https://47.98.136.66:8070/" ;
char OTA_BIN_File[] = "ethernet_basic.bin" ;
char OTA_URL[] = "" ;
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");
/**
 * @name:
 * @brief:
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录：初次创建
 */
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(OTA_TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}
/**
 * @name:Stop_Taskwdt_OTATask
 * @brief:关闭任务看门狗
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录：初次创建
 */
void Stop_Taskwdt_OTATask()
{
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
}
/**
 * @name:simple_ota_example_task
 * @brief:OTA任务
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录：初次创建
 */
bool OTA_Flag = false;
uint8_t OTA_wdt_Status = 0;
void simple_ota_example_task(void *pvParameter)
{
    ESP_LOGI(OTA_TAG, "Starting OTA example");
    strcat(OTA_URL, OTA_Adder_IP);
    strcat(OTA_URL, OTA_BIN_File);
    printf("OTA_URL is %s \r\n", OTA_URL);
    esp_http_client_config_t config =
    {
        .url = OTA_URL,
        .cert_pem = (char *)server_cert_pem_start,
        .event_handler = _http_event_handler,
    };
#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
    char url_buf[OTA_URL_SIZE];
    if (strcmp(config.url, "FROM_STDIN") == 0)
    {
        example_configure_stdin_stdout();
        fgets(url_buf, OTA_URL_SIZE, stdin);
        int len = strlen(url_buf);
        url_buf[len - 1] = '\0';
        config.url = url_buf;
    }
    else
    {
        ESP_LOGE(OTA_TAG, "Configuration mismatch: wrong firmware upgrade image url");
        abort();
    }
#endif
#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif
    //设置Task看门狗的超时时间-（10分钟）
    esp_task_wdt_init((CONFIG_EXAMPLE_OTA_RECV_TIMEOUT / OneSec_RTOS), OTA_Flag);
    esp_task_wdt_add(&OTA_Handler);
    OTA_wdt_Status = esp_task_wdt_status(&OTA_Handler);
    //喂狗
    esp_task_wdt_reset();
    printf("OTA_wdt_Status is %d \r\n", OTA_wdt_Status);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK)
    {
        esp_restart();
    }
    else
    {
        ESP_LOGE(OTA_TAG, "Firmware upgrade failed");
    }
    while (1)
    {
        //喂狗
        esp_task_wdt_reset();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
    {
        ESP_LOGI(OTA_TAG, "Running firmware version: %s", running_app_info.version);
    }
    printf("Running ver is %s \r\n",running_app_info.version);
    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0)
    {
        ESP_LOGW(OTA_TAG, "Current running version is the same as a new. We will not continue the update.");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void advanced_ota_example_task(void *pvParameter)
{
    ESP_LOGI(OTA_TAG, "Starting Advanced OTA example");
    strcat(OTA_URL, OTA_Adder_IP);
    strcat(OTA_URL, OTA_BIN_File);
    printf("OTA_URL is %s \r\n", OTA_URL);
    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t config =
    {
        .url = OTA_URL,
        .cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT,
    };
#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
    char url_buf[OTA_URL_SIZE];
    if (strcmp(config.url, "FROM_STDIN") == 0)
    {
        example_configure_stdin_stdout();
        fgets(url_buf, OTA_URL_SIZE, stdin);
        int len = strlen(url_buf);
        url_buf[len - 1] = '\0';
        config.url = url_buf;
    }
    else
    {
        ESP_LOGE(OTA_TAG, "Configuration mismatch: wrong firmware upgrade image url");
        abort();
    }
#endif
#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif
    esp_https_ota_config_t ota_config =
    {
        .http_config = &config,
    };
    esp_https_ota_handle_t https_ota_handle = NULL;
    //设置Task看门狗的超时时间-（10分钟）
    esp_task_wdt_init((CONFIG_EXAMPLE_OTA_RECV_TIMEOUT / OneSec_RTOS), OTA_Flag);
    esp_task_wdt_add(&OTA_Handler);
    OTA_wdt_Status = esp_task_wdt_status(&OTA_Handler);
    //喂狗
    esp_task_wdt_reset();
    printf("OTA_wdt_Status is %d \r\n", OTA_wdt_Status);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(OTA_TAG, "ESP HTTPS OTA Begin failed");
        vTaskDelete(NULL);
    }
    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK)
    {
        ESP_LOGE(OTA_TAG, "esp_https_ota_read_img_desc failed");
        goto ota_end;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK)
    {
        ESP_LOGE(OTA_TAG, "image header verification failed");
        goto ota_end;
    }
    printf(" version is %s \r\n", app_desc.version);
    //printf(" project_name is %s \r\n", app_desc.project_name);
    //printf(" time is %s \r\n", app_desc.time);
    //printf(" date is %s \r\n", app_desc.date);
    //printf(" idf_ver is %s \r\n", app_desc.idf_ver);
    while (1)
    {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
        {
            break;
        }
        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
        ESP_LOGD(OTA_TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
    }
    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true)
    {
        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(OTA_TAG, "Complete data was not received.");
    }
ota_end:
    ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if ((err == ESP_OK) && (ota_finish_err == ESP_OK))
    {
        ESP_LOGI(OTA_TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    }
    else
    {
        if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED)
        {
            ESP_LOGE(OTA_TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(OTA_TAG, "ESP_HTTPS_OTA upgrade failed %d", ota_finish_err);
        vTaskDelete(NULL);
    }
}