#include "wifi.h"
#include "config.h"
#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"

#ifndef PROJECT_WIFI_RETRY_DELAY_MS
#define PROJECT_WIFI_RETRY_DELAY_MS 3000
#endif

static const char *TAG = "WiFi";

static void reconnect(TimerHandle_t xTimer)
{
    esp_wifi_connect();
}

static TimerHandle_t wifi_reconnect_timer = xTimerCreate("wifi_reconnect_timer", pdMS_TO_TICKS(PROJECT_WIFI_RETRY_DELAY_MS), pdFALSE, NULL, &reconnect);

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_config_t wifi_config;
        ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
        xTimerStart(wifi_reconnect_timer, portMAX_DELAY);
        ESP_LOGI(TAG, "connect to the AP '%s' fail, reconnect after %d seconds ...", wifi_config.sta.ssid, PROJECT_WIFI_RETRY_DELAY_MS / 1000);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        wifi_config_t wifi_config;
        ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
        ESP_LOGI(TAG, "connect to the AP '%s' success", wifi_config.sta.ssid);
    }
}

void wifi_sta_init()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_sta_init finished.");
}

void wifi_sta_config(const std::string& s_ssid, const std::string& s_password)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
    strcpy((char *)(wifi_config.sta.ssid), s_ssid.c_str());
    strcpy((char *)(wifi_config.sta.password), s_password.c_str());
    if (s_password.length() == 0)
    {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}