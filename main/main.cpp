#include <stdio.h>
#include "version.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "esp_wifi.h"
#include "config.h"
#include "lwip/sockets.h"
#include "udp_server.h"
#include "miscellaneous.h"
#include "models/settings.h"
#include "models/info.h"
#include "models/status.h"
#include "port.h"

static const char *TAG = "Main";

static void hello_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Hello Task starts");
    while(true)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Sleep 10 seconds");
        ESP_LOGI(TAG, "Free Heap %d Kbytes", heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) >> 10);
        // Settings::GetInstance().Log();
        // InfoModel::GetInstance().Log();
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
                delete json_string;
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

static void dmx_message_handler(const char * msg, size_t len, const char * sender)
{
    ESP_LOGI(TAG, "dmx_message_handler");
}

static void artsync_message_handler(const char * msg, size_t len, const char * sender)
{
    ESP_LOGI(TAG, "artsync_message_handler");
}

static void discovery_message_handler(const char * msg, size_t len, const char * sender)
{
    ESP_LOGI(TAG, "discovery_message_handler");
}

static void common_message_handler(const char * msg, size_t len, const char * sender)
{
    ESP_LOGI(TAG, "common_message_handler");
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ArtNet Node - Full Master Wireless");
    FWVersion::log();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    HWStatus::Mode mode = HWStatus::GetMode();
    if (mode == HWStatus::Mode::CONFIG_ONLY)
    {
        WifiAP::GetInstance().Init();
        CommonServer::GetInstance().RegisterMessageHandler(common_message_handler);
        xTaskCreate(CommonServer::FreeRTOSTask, "CommonServer::FreeRTOSTask", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    }
    else if (mode == HWStatus::Mode::CONFIG_AND_RUN_DMX)
    {
        WifiSTA::GetInstance().Init();

        ArtNetServer::GetInstance().RegisterDMXMessageHandler(dmx_message_handler);
        ArtNetServer::GetInstance().RegisterArtSyncMessageHandler(artsync_message_handler);
        ArtNetServer::GetInstance().RegisterDiscoveryMessageHandler(discovery_message_handler);
        CommonServer::GetInstance().RegisterMessageHandler(common_message_handler);

        xTaskCreate(ArtNetServer::FreeRTOSTask, "ArtNetServer::FreeRTOSTask", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
        xTaskCreate(CommonServer::FreeRTOSTask, "CommonServer::FreeRTOSTask", 4096, NULL, configMAX_PRIORITIES - 2, NULL);
    }

    xTaskCreate(hello_task, "hello_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    // xTaskCreate(test_json_task, "test_json_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    // xTaskCreate(test_nvs_task, "test_nvs_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    // xTaskCreate(test_nvs_and_json_task, "test_nvs_and_json_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
}
