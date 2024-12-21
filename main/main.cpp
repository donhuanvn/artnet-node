#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "cJSON.h"

static const char *TAG = "ArtNet";

static void hello_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Hello Task starts");
    while(true)
    {
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Sleep 3 seconds");
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

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ArtNet Node - Full Master Wireless");
    xTaskCreate(hello_task, "hello_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(test_json_task, "test_json_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
}
