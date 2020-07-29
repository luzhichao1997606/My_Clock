/*
 * @file:
 * @Descripttion:
 * @brief:
 * @version:
 * @author: lzc
 * @attention: 注意：
 * @Date: 2020-07-18 17:37:34
 * @LastEditors: lzc
 * @LastEditTime: 2020-07-29 18:16:24
 */
#include "https_ota.h"

#define OTA_URL_SIZE 256
#define CONFIG_EXAMPLE_OTA_RECV_TIMEOUT     600*1000 //10分钟超时时间
#define OneSec_RTOS                         1000     //1秒计时

#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */

char OTA_URL[200] = "" ;

//HTTPS服务器
char OTA_Adder_IP[100] = "https://cdn.virtualbing.cn/";
char OTA_BIN_File[100] = "hello-world.bin";

//TODO:数据需要合成
//合宙的服务器的IP，数据需要合成
char OTA_HTTP_HZ_URL[500] = "http://iot.openluat.com/api/site/firmware_upgrade?project_key=4FXYhaL8seFmIlLoY87wgKQycW4sx4CJ&imei=865373047839743&device_key=kC5zJV6136915258&firmware_name=My_Clock&version=1.1.0&need_oss_url=1";

//自己的服务器
char OTA_Test_Adder[100] = "http://47.98.136.66/";
char OTA_Test_BIN[100] = "hello-world.bin";

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "iot.openluat.com"
#define WEB_PORT 80
#define WEB_URL "/api/site/firmware_upgrade?project_key=4FXYhaL8seFmIlLoY87wgKQycW4sx4CJ&imei=865373047839743&device_key=kC5zJV6136915258&firmware_name=My_Clock&version=1.1.0&need_oss_url=1"

//HTTP发送数据的缓存区
static const char *REQUEST    = "GET " WEB_URL " HTTP/1.1\r\n"
                                "Host: "WEB_SERVER"\r\n"
                                "Connection: keep-alive\r\n"
                                "\r\n";

static char ota_write_data[BUFFSIZE + 1] = { 0 };
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");
/**
 * @name:_http_event_handler
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
 * @brief:OTA任务——HTTPS
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

/**
 * @name: validate_image_header
 * @brief: https处理函数
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录 初次创建
 */
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
    printf("Running ver is %s \r\n", running_app_info.version);
    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0)
    {
        ESP_LOGW(OTA_TAG, "Current running version is the same as a new. We will not continue the update.");
        return ESP_FAIL;
    }
    return ESP_OK;
}
/**
 * @name: advanced_ota_example_task
 * @brief: 加版本校验的HTTPS服务OTA
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录 初次创建
 */
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
/**
 * @name: http_cleanup
 * @brief: 除去HTTP的连接
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录 初次创建
 */
static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}
/**
 * @name: 
 * @brief: 
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录 初次创建
 */
static void __attribute__((noreturn)) task_fatal_error()
{
    ESP_LOGE(OTA_TAG, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);
    while (1)
    {
        ;
    }
}
/**
 * @name: print_sha256
 * @brief: SHA256校验
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录 初次创建
 */
static void print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i)
    {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(OTA_TAG, "%s: %s", label, hash_print);
}

/**
 * @name: infinite_loop
 * @brief: OTA无更新版本进入此函数
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录 初次创建
 */
static void infinite_loop(void)
{
    int i = 0;
    ESP_LOGI(OTA_TAG, "When a new firmware is available on the server, press the reset button to download it");
    while (1)
    {
        ESP_LOGI(OTA_TAG, "Waiting for a new firmware ... %d", ++i);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        if (i >= 10)
        {
            ESP_LOGI(OTA_TAG, "NO new firmware ... ");
            //esp_restart();
            //vTaskDelete(OTA_Handler);
            printf("挂起OTA任务！\r\n");
            vTaskSuspend(OTA_Handler);
            break;
        }
    }
}

/**
 * @name: http_get_task
 * @brief: 获取URL连接的任务
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录 初次创建
 */
char HZ_HTTP_URL_Get[120] = {0};
bool Over_Flag = false;
static void http_get_task(void *pvParameters)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    const struct addrinfo hints =
    {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[500];
    while (1)
    {
        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);
        if (err != 0 || res == NULL)
        {
            ESP_LOGE(OTA_TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(OTA_TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));
        s = socket(res->ai_family, res->ai_socktype, 0);
        if (s < 0)
        {
            ESP_LOGE(OTA_TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(OTA_TAG, "... allocated socket");
        if (connect(s, res->ai_addr, res->ai_addrlen) != 0)
        {
            ESP_LOGE(OTA_TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(OTA_TAG, "... connected");
        freeaddrinfo(res);
        if (write(s, REQUEST, strlen(REQUEST)) < 0)
        {
            ESP_LOGE(OTA_TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(OTA_TAG, "... socket send success");
        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                       sizeof(receiving_timeout)) < 0)
        {
            ESP_LOGE(OTA_TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(OTA_TAG, "... set socket receiving timeout success");
        /* Read HTTP response */
        do
        {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf) - 1);
            for (int i = 0; i < r; i++)
            {
                putchar(recv_buf[i]);
            }
            char Source[150], Proc[50], Output[150];
            bzero(Source, sizeof(Source));
            bzero(Proc, sizeof(Proc));
            bzero(Output, sizeof(Output));
            //
            strcpy(Source, strstr(recv_buf, "Location"));
            strcpy(Proc, strstr(recv_buf, "Air-UpgradeTime"));
            //
            printf("1Count ++ ,%s,%d\r\n", Source, strlen(Source));
            printf("2Count ++ ,%s,%d\r\n", Proc, strlen(Proc));
            //
            memmove(Output, Source, strlen(Source) -  strlen(Proc) - 2);
            printf("3Count ++ ,%s,%d\r\n", Output, strlen(Output));
            //
            strcpy(Output, strstr(Output, "http"));
            printf("4Count ++ ,%s,%d\r\n", Output, strlen(Output));
            //
            strcpy(HZ_HTTP_URL_Get, Output);
            break;
        }
        while (r > 0);
        ESP_LOGI(OTA_TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        ESP_LOGI(OTA_TAG, "END!!!");
        Over_Flag = true;
        vTaskDelete(NULL);
    }
}
/**
 * @name: ota_example_task
 * @brief: OTA_http 支持合宙平台。
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录 初次创建
 */
void ota_example_task(void *pvParameter)
{
    esp_err_t err;
    bool Connect_With_HZ_Flag = false;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    esp_http_client_handle_t client;
    const esp_partition_t *update_partition = NULL;
    Check_SHA256();
    ESP_LOGI(OTA_TAG, "Starting OTA example");
    strcat(OTA_URL, OTA_Test_Adder);
    strcat(OTA_URL, OTA_Test_BIN);
    printf("OTA_URL is %s \r\n", OTA_URL);
    printf("OTA_HTTP_HZ_URL is %s \r\n", OTA_HTTP_HZ_URL);
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();
    printf("Ver is 1.0.0 \r\n");
    //设置Task看门狗的超时时间-（10分钟）
    esp_task_wdt_init((CONFIG_EXAMPLE_OTA_RECV_TIMEOUT / OneSec_RTOS), OTA_Flag);
    esp_task_wdt_add(&OTA_Handler);
    OTA_wdt_Status = esp_task_wdt_status(&OTA_Handler);
    //喂狗
    esp_task_wdt_reset();
    printf("OTA_wdt_Status is %d \r\n", OTA_wdt_Status);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    if (configured != running)
    {
        ESP_LOGW(OTA_TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x", configured->address, running->address);
        ESP_LOGW(OTA_TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(OTA_TAG, "Running partition type %d subtype %d (offset 0x%08x)", running->type, running->subtype, running->address);
    esp_http_client_config_t config =
    {
        .url = OTA_HTTP_HZ_URL,
        .cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT,
        .buffer_size = 4096,
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
//用于获取URL
    esp_http_client_handle_t clients = esp_http_client_init(&config);
    if (clients == NULL)
    {
        ESP_LOGE(OTA_TAG, "Failed to initialise HTTP connection");
        task_fatal_error();
    }
    err = esp_http_client_open(clients, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(OTA_TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(clients);
        task_fatal_error();
    }
    esp_http_client_fetch_headers(clients);
    printf("esp_http_client_get_status_code(clients) %d \r\n", esp_http_client_get_status_code(clients));
    ///创建合宙平台线程进行重定向工作
    if (esp_http_client_get_status_code(clients) == 302)
    {
        Connect_With_HZ_Flag = true;
        xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
        while (!Over_Flag)
        {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
//OTA
    http_cleanup(clients);
    client = esp_http_client_init(&config);
    //将重定向的URL放入HTTP连接设置中
    if (Connect_With_HZ_Flag)
    {
        esp_http_client_set_url(client, HZ_HTTP_URL_Get);
        Connect_With_HZ_Flag = false;
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(OTA_TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
    }
    esp_http_client_fetch_headers(client);
    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(OTA_TAG, "Writing to partition subtype %d at offset 0x%x", update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);
    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    while (1)
    {
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0)
        {
            ESP_LOGE(OTA_TAG, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
        }
        else if (data_read > 0)
        {
            if (image_header_was_checked == false)
            {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
                {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(OTA_TAG, "New firmware version: %s", new_app_info.version);
                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
                    {
                        ESP_LOGI(OTA_TAG, "Running firmware version: %s", running_app_info.version);
                    }
                    const esp_partition_t *last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK)
                    {
                        ESP_LOGI(OTA_TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                    }
                    // check current version with last invalid partition
                    if (last_invalid_app != NULL)
                    {
                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0)
                        {
                            ESP_LOGW(OTA_TAG, "New version is the same as invalid version.");
                            ESP_LOGW(OTA_TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                            ESP_LOGW(OTA_TAG, "The firmware has been rolled back to the previous version.");
                            http_cleanup(client);
                            infinite_loop();
                        }
                    }
                    //当前运行的版本与新的相同。我们将不会继续更新
                    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0)
                    {
                        ESP_LOGW(OTA_TAG, "Current running version is the same as a new. We will not continue the update.");
                        http_cleanup(client);
                        infinite_loop();
                    }
                    //当前运行的版本比平台版本更加新。我们将不会继续更新
                    printf("new_app_info.version IS %s \r\n", new_app_info.version);
                    printf("running_app_info.version IS %s \r\n", running_app_info.version);
                    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) < 0)
                    {
                        ESP_LOGW(OTA_TAG, "Current running version is newest. We will not continue the update.");
                        printf("running_app_info.version is newest\r\n");
                        http_cleanup(client);
                        infinite_loop();
                    }
                    image_header_was_checked = true;
                    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(OTA_TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        task_fatal_error();
                    }
                    ESP_LOGI(OTA_TAG, "esp_ota_begin succeeded");
                }
                else
                {
                    ESP_LOGE(OTA_TAG, "received package is not fit len");
                    http_cleanup(client);
                    task_fatal_error();
                }
            }
            err = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK)
            {
                http_cleanup(client);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGD(OTA_TAG, "Written image length %d", binary_file_length);
        }
        else if (data_read == 0)
        {
            /*
             * As esp_http_client_read never returns negative error code, we rely on
             * `errno` to check for underlying transport connectivity closure if any
             */
            if (errno == ECONNRESET || errno == ENOTCONN)
            {
                ESP_LOGE(OTA_TAG, "Connection closed, errno = %d", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true)
            {
                ESP_LOGI(OTA_TAG, "Connection closed");
                break;
            }
        }
    }
    ESP_LOGI(OTA_TAG, "Total Write binary data length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true)
    {
        ESP_LOGE(OTA_TAG, "Error in receiving complete file");
        http_cleanup(client);
        task_fatal_error();
    }
    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED)
        {
            ESP_LOGE(OTA_TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(OTA_TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(OTA_TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(OTA_TAG, "Prepare to restart system!");
    esp_restart();
    return ;
}
/**
 * @name: diagnostic
 * @brief: 诊断函数
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录 初次创建
 */
static bool diagnostic(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type    = GPIO_PIN_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << 4);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    ESP_LOGI(OTA_TAG, "Diagnostics (5 sec)...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    bool diagnostic_is_ok = gpio_get_level(4);
    gpio_reset_pin(4);
    return diagnostic_is_ok;
}
/**
 * @name: Check_SHA256
 * @brief: SHA256校验函数
 * @author: lzc
 * @param {type} None
 * @return: None
 * @note: 修改记录 初次创建
 */
void Check_SHA256(void)
{
    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition ;
    // get sha256 digest for the partition table
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table: ");
    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");
    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK)
    {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
        {
            // run diagnostic function ...
            bool diagnostic_is_ok = diagnostic();
            if (diagnostic_is_ok)
            {
                ESP_LOGI(OTA_TAG, "Diagnostics completed successfully! Continuing execution ...");
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else
            {
                ESP_LOGE(OTA_TAG, "Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
}
