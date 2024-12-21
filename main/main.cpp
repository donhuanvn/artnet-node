#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"

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

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ArtNet Node - Full Master Wireless");
    xTaskCreate(hello_task, "hello_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
}
