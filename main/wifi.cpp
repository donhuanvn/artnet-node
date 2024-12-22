#include "wifi.h"
#include "config.h"
#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_mac.h"

#ifndef PROJECT_WIFI_RETRY_DELAY_MS
#define PROJECT_WIFI_RETRY_DELAY_MS 3000
#endif

static const char *TAG_STA = "WiFi-STA";
static const char *TAG_AP = "WiFi-AP";

class ChangeWifiMode
{
    wifi_mode_t mode;
public:
    ChangeWifiMode(wifi_mode_t destMode)
    {
        ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
        ESP_ERROR_CHECK(esp_wifi_set_mode(destMode));
    }
    ~ChangeWifiMode()
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    }
};

static bool is_valid_ssid(const std::string& s_ssid)
{
    return s_ssid.length() > 1;
}

static bool is_valid_password(const std::string& s_password, wifi_auth_mode_t& o_auth_mode)
{
    bool valid = false;
    valid |= (o_auth_mode == WIFI_AUTH_OPEN && s_password.empty());
    valid |= (o_auth_mode == WIFI_AUTH_WPA_WPA2_PSK && s_password.length() >= 8);
    return valid;
}

static void reconnect(TimerHandle_t xTimer)
{
    esp_wifi_connect();
}

static TimerHandle_t wifi_reconnect_timer = xTimerCreate("wifi_reconnect_timer", pdMS_TO_TICKS(PROJECT_WIFI_STA_RETRY_DELAY_MS), pdFALSE, NULL, &reconnect);

void wifi_sta_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
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
        ESP_LOGI(TAG_STA, "connect to the AP '%s' fail, reconnect after %d seconds ...", wifi_config.sta.ssid, PROJECT_WIFI_STA_RETRY_DELAY_MS / 1000);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        char buffer[20];
        sprintf(buffer, IPSTR, IP2STR(&event->ip_info.ip));
        WifiSTA::GetInstance().sGotIP = buffer;

        wifi_config_t wifi_config;
        ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
        ESP_LOGI(TAG_STA, "connect to the AP '%s' success", wifi_config.sta.ssid);
    }
}

static void wifi_ap_event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG_AP, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG_AP, "station " MACSTR " leave, AID=%d, reason=%d", MAC2STR(event->mac), event->aid, event->reason);
    }
}

void WifiSTA::Init()
{
    if (WifiAP::GetInstance().HasInit())
    {
        WifiAP::GetInstance().Deinit();
    }

    netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL, &ins_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL, &ins_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_STA, "wifi_sta_init finished.");

    bInit = true;
}

bool WifiSTA::HasInit()
{
    return bInit;
}

void WifiSTA::Deinit()
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, ins_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ins_got_ip));
    esp_netif_destroy_default_wifi(netif);
    bInit = false;
}

void WifiSTA::Config(const std::string &s_ssid, const std::string &s_password)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
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
    ChangeWifiMode _(WIFI_MODE_STA);
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Error (%s)", esp_err_to_name(err));
    }
    ESP_ERROR_CHECK(err);
}

bool WifiSTA::HasValidConfig()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config;
    ChangeWifiMode _(WIFI_MODE_STA);
    esp_err_t err = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Error (%s)", esp_err_to_name(err));
    }
    ESP_ERROR_CHECK(err);

    bool valid = true;
    valid &= is_valid_ssid((const char *)(wifi_config.sta.ssid));
    valid &= is_valid_password((const char *)(wifi_config.sta.password), wifi_config.sta.threshold.authmode);

    return valid;
}

std::pair<std::string, std::string> WifiSTA::GetConfig()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config;
    ChangeWifiMode _(WIFI_MODE_STA);
    esp_err_t err = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Error (%s)", esp_err_to_name(err));
    }
    ESP_ERROR_CHECK(err);

    return std::make_pair(std::string((const char *)(wifi_config.sta.ssid)), std::string((const char *)(wifi_config.sta.password)));
}

void WifiAP::Init()
{
    if (WifiSTA::GetInstance().HasInit())
    {
        WifiSTA::GetInstance().Deinit();
    }

    netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler, NULL, &ins_any_id));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_config_t wifi_config;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGI(TAG_AP, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.channel);

    bInit = true;
}

bool WifiAP::HasInit()
{
    return bInit;
}

void WifiAP::Deinit()
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, ins_any_id));
    esp_netif_destroy_default_wifi(netif);
    bInit = false;
}

void WifiAP::Config(const std::string &s_ssid, const std::string &s_password)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    strcpy((char *)(wifi_config.ap.ssid), s_ssid.c_str());
    wifi_config.ap.ssid_len = s_ssid.length();
    strcpy((char *)(wifi_config.ap.password), s_password.c_str());
    wifi_config.ap.channel = PROJECT_WIFI_AP_CHANNEL;
    wifi_config.ap.max_connection = PROJECT_WIFI_AP_MAX_CONN;
    wifi_config.ap.pmf_cfg.required = false; // Cannot set PMF to required when in WIFI_AUTH_WPA_WPA2_PSK! Setting PMF to optional.
    if (s_password.length() == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }
    ChangeWifiMode _(WIFI_MODE_AP);
    esp_err_t err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Error (%s)", esp_err_to_name(err));
    }
    ESP_ERROR_CHECK(err);
}

bool WifiAP::HasValidConfig()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config;
    ChangeWifiMode _(WIFI_MODE_AP);
    esp_err_t err = esp_wifi_get_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_AP, "Error (%s)", esp_err_to_name(err));
    }
    ESP_ERROR_CHECK(err);

    bool valid = true;
    valid &= is_valid_ssid((const char *)(wifi_config.ap.ssid));
    valid &= is_valid_password((const char *)(wifi_config.ap.password), wifi_config.ap.authmode);
    valid &= (wifi_config.ap.channel == PROJECT_WIFI_AP_CHANNEL);
    valid &= (wifi_config.ap.max_connection == PROJECT_WIFI_AP_MAX_CONN);
    valid &= (wifi_config.ap.pmf_cfg.required == false);

    return valid;
}

std::pair<std::string, std::string> WifiAP::GetConfig()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config;
    ChangeWifiMode _(WIFI_MODE_AP);
    esp_err_t err = esp_wifi_get_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Error (%s)", esp_err_to_name(err));
    }
    ESP_ERROR_CHECK(err);

    return std::make_pair(std::string((const char *)(wifi_config.ap.ssid)), std::string((const char *)(wifi_config.ap.password)));
}