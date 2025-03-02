#include "wifi.h"
#include "config.h"
#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_mac.h"
#include "lwip/inet.h"
#include "models/settings.h"

#ifndef PROJECT_WIFI_RETRY_DELAY_MS
#define PROJECT_WIFI_RETRY_DELAY_MS 3000
#endif

static const char *TAG_STA = "WiFi-STA";
static const char *TAG_AP = "WiFi-AP";

bool WifiAP::m_bStarted = false;
esp_event_handler_instance_t WifiAP::m_ins_any_id = nullptr;
esp_netif_t * WifiAP::netif = nullptr;

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
        WifiSTA::GetInstance().m_sGotIP = inet_ntoa(event->ip_info.ip);
        WifiSTA::GetInstance().m_sGotNetmask = inet_ntoa(event->ip_info.netmask);
        WifiSTA::GetInstance().m_sGatewayAddress = inet_ntoa(event->ip_info.gw);
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
    m_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL, &m_ins_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL, &m_ins_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_STA, "wifi_sta_init finished.");

    m_bInit = true;
}

bool WifiSTA::HasInit()
{
    return m_bInit;
}

void WifiSTA::Deinit()
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, m_ins_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, m_ins_got_ip));
    esp_netif_destroy_default_wifi(m_netif);
    m_bInit = false;
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

esp_err_t WifiAP::Start()
{
    if (IsStarted())
    {
        ESP_LOGE(TAG_AP, "AP already started");
        return ESP_ERR_INVALID_STATE;
    }

    netif = esp_netif_create_default_wifi_ap();
    wifi_init_config_t stInitCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&stInitCfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler, NULL, &m_ins_any_id));

    wifi_config_t stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    const std::string& sSsid = Settings::GetInstance().GetBroadcastSSID();
    const std::string& sPass = Settings::GetInstance().GetBroadcastPassword();
    strcpy((char *)(stCfg.ap.ssid), sSsid.c_str());
    stCfg.ap.ssid_len = sSsid.length();
    strcpy((char *)(stCfg.ap.password), sPass.c_str());
    stCfg.ap.channel = PROJECT_WIFI_AP_CHANNEL;
    stCfg.ap.max_connection = PROJECT_WIFI_AP_MAX_CONN;
    stCfg.ap.pmf_cfg.required = false; // Cannot set PMF to required when in WIFI_AUTH_WPA_WPA2_PSK! Setting PMF to optional.
    stCfg.ap.authmode = (sPass.length() == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK;
    esp_err_t err = esp_wifi_set_config(WIFI_IF_AP, &stCfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_AP, "Failed to set wifi config (%s)", esp_err_to_name(err));
        return err;
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    m_bStarted = true;

    ESP_LOGI(TAG_AP, "Wifi Access-Point mode is started. SSID: '%s' password: '%s' channel: %d", stCfg.ap.ssid, stCfg.ap.password, stCfg.ap.channel);

    return ESP_OK;
}

bool WifiAP::IsStarted()
{
    wifi_mode_t mode;
    switch (esp_wifi_get_mode(&mode))
    {
    case ESP_ERR_WIFI_NOT_INIT:
        return false;
    case ESP_ERR_INVALID_ARG:
        ESP_LOGE(TAG_AP, "Invalid argument to get wifi mode");
        return false;
    }
    return m_bStarted && netif != nullptr && mode == WIFI_MODE_AP;
}

esp_err_t WifiAP::Stop()
{
    if (!IsStarted())
    {
        ESP_LOGE(TAG_AP, "AP not started");
        return ESP_ERR_INVALID_STATE;
    }

    if (esp_wifi_stop() != ESP_OK)
    {
        ESP_LOGE(TAG_AP, "Failed to stop AP");
    }
    if (esp_wifi_deinit() != ESP_OK)
    {
        ESP_LOGE(TAG_AP, "Failed to deinit AP");
    }
    if (esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, m_ins_any_id) != ESP_OK)
    {
        ESP_LOGE(TAG_AP, "Failed to unregister event handler");
    }
    if (esp_wifi_deinit() != ESP_OK)
    {
        ESP_LOGE(TAG_AP, "Failed to deinit wifi");
    }

    esp_netif_destroy_default_wifi(netif);
    netif = nullptr;

    return ESP_OK;
}

int32_t WifiAP::GetClientsCount()
{
    if (!IsStarted())
    {
        ESP_LOGE(TAG_AP, "AP not started");
        return -1;
    }

    static wifi_sta_list_t stList;
    if (esp_wifi_ap_get_sta_list(&stList) != ESP_OK)
    {
        ESP_LOGE(TAG_AP, "Failed to get AP STA list");
        return -1;
    }

    return stList.num;
}