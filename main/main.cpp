#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "esp_wifi.h"
#include "config.h"
#include "lwip/sockets.h"
#include "udp_server.h"

static const char *TAG = "Main";

static void hello_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Hello Task starts");
    while(true)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Sleep 10 seconds");
    }
}

static void test_json_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Test JSON Task starts");
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "name", "John Doe");
    cJSON_AddNumberToObject(json, "age", 30);
    cJSON_AddStringToObject(json, "email", "john.doe@example.com");
    cJSON_AddItemToObject(json, "address", cJSON_CreateString("Danang, Vietnam"));

    while(true)
    {
        ESP_LOGI(TAG, "JSON String is \n'%s'", cJSON_Print(json));
        cJSON * pItemAge = cJSON_GetObjectItemCaseSensitive(json, "age");
        if (cJSON_IsNumber(pItemAge) && pItemAge->valueint > 0)
        {
            cJSON_ReplaceItemInObject(json, "age", cJSON_CreateNumber(pItemAge->valueint + 1));
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

static void test_nvs_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Test NVS Task starts");
    
    while (true)
    {
        ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle...");
        nvs_handle_t my_handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "Done!");

            int32_t s32TestValue = 0;
            err = nvs_get_i32(my_handle, "test_value", &s32TestValue);
            switch (err)
            {
            case ESP_OK:
                ESP_LOGI(TAG, "Test Value = %"PRIi32"", s32TestValue);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGE(TAG, "The value is not initialized yet!");
                break;
            default:
                ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
                break;
            }

            ESP_LOGI(TAG, "Updating the value in NVS ...");
            err = nvs_set_i32(my_handle, "test_value", s32TestValue + 1);
            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "Done");
            }
            else
            {
                ESP_LOGE(TAG, "ERROR!");
            }
        }

        ESP_LOGI(TAG, "Committing updates in NVS ...");
        err = nvs_commit(my_handle);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Done");
        }
        else
        {
            ESP_LOGE(TAG, "ERROR!");
        }

        nvs_close(my_handle);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

static void test_nvs_and_json_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Test NVS and JSON Task starts");

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "name", "John Doe");
    cJSON_AddNumberToObject(json, "age", 30);
    cJSON_AddStringToObject(json, "email", "john.doe@example.com");
    cJSON_AddItemToObject(json, "address", cJSON_CreateString("Danang, Vietnam"));

    while (true)
    {
        ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle...");
        nvs_handle_t my_handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "Done!");

            int32_t s32TestValue = 0;
            err = nvs_get_i32(my_handle, "test_value", &s32TestValue);


            size_t len = 0;
            err = nvs_get_str(my_handle, "test_string", NULL, &len);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "ERROR!");
            }
            else
            {
                char * json_string = (char *) malloc(len);
                err = nvs_get_str(my_handle, "test_string", json_string, &len);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "ERROR!");
                }
                else
                {
                    if (json != NULL)
                    {
                        cJSON_Delete(json);
                        json = NULL;
                    }
                    json = cJSON_Parse(json_string);
                    if (json == NULL)
                    {
                        ESP_LOGE(TAG, "ERROR!");
                    }
                    else
                    {
                        ESP_LOGI(TAG, "JSON String is \n'%s'", cJSON_Print(json));
                        cJSON * pItemAge = cJSON_GetObjectItemCaseSensitive(json, "age");
                        if (cJSON_IsNumber(pItemAge) && pItemAge->valueint > 0)
                        {
                            cJSON_ReplaceItemInObject(json, "age", cJSON_CreateNumber(pItemAge->valueint + 1));
                        }
                    }
                }
            }
        }

        ESP_LOGI(TAG, "Updating the string in NVS ...");
        err = nvs_set_str(my_handle, "test_string", cJSON_PrintUnformatted(json));
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Done");

            cJSON_Delete(json);
            json = NULL;

            ESP_LOGI(TAG, "Committing updates in NVS ...");
            err = nvs_commit(my_handle);
            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "Done");
            }
            else
            {
                ESP_LOGE(TAG, "ERROR!");
            }
        }
        else
        {
            ESP_LOGE(TAG, "ERROR!");
        }

        nvs_close(my_handle);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

static void udp_server_artnet_task(void *pvParameters)
{
    ESP_LOGI(TAG, "UDP Server ArtNet Task starts");

    char addr_str[128];
    struct sockaddr_in dest_addr = {}; // IPv4
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PROJECT_UDP_ARTNET_PORT); // To read later: __builtin_bswap16

    while (true)
    {
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 3600;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PROJECT_UDP_ARTNET_PORT);

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);

        while (true)
        {
            char * buffer = ArtNetServer::GetInstance().GetBuffer();
            size_t bufferlen = ArtNetServer::GetInstance().GetBufferLength() - 1;
            int len = recvfrom(sock, buffer, bufferlen, 0, (struct sockaddr *)&source_addr, &socklen);
            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else if (source_addr.ss_family == PF_INET)
            {
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                ArtNetServer::GetInstance().HandleIncommingMessage(len, addr_str);
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
}

static void udp_server_common_task(void *pvParameters)
{
    ESP_LOGI(TAG, "UDP Server Common Task starts");

    char addr_str[128];
    struct sockaddr_in dest_addr = {}; // IPv4
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PROJECT_UDP_COMMON_PORT); // To read later: __builtin_bswap16

    while (true)
    {
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 3600;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PROJECT_UDP_COMMON_PORT);

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);

        while (true)
        {
            char * buffer = CommonServer::GetInstance().GetBuffer();
            size_t bufferlen = CommonServer::GetInstance().GetBufferLength() - 1;
            int len = recvfrom(sock, buffer, bufferlen, 0, (struct sockaddr *)&source_addr, &socklen);
            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else if (source_addr.ss_family == PF_INET)
            {
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                CommonServer::GetInstance().HandleIncommingMessage(len, addr_str);
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ArtNet Node - Full Master Wireless");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    WifiSTA::GetInstance().Init();

    xTaskCreate(hello_task, "hello_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    // xTaskCreate(test_json_task, "test_json_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    // xTaskCreate(test_nvs_task, "test_nvs_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    // xTaskCreate(test_nvs_and_json_task, "test_nvs_and_json_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(udp_server_artnet_task, "udp_server_artnet_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(udp_server_common_task, "udp_server_common_task", 4096, NULL, configMAX_PRIORITIES - 2, NULL);
}
