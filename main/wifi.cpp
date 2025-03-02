#include "wifi.h"
#include "config.h"
#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_mac.h"
#include "models/settings.h"
#include "esp_netif.h"
#include "ping/ping_sock.h"

#ifndef PROJECT_WIFI_RETRY_DELAY_MS
#define PROJECT_WIFI_RETRY_DELAY_MS 3000
#endif

static const char *TAG_STA = "WiFi-STA";
static const char *TAG_AP = "WiFi-AP";

bool WifiSTA::m_bStarted = false;
esp_event_handler_instance_t WifiSTA::m_ins_any_id = nullptr;
esp_event_handler_instance_t WifiSTA::m_ins_got_ip = nullptr;
esp_netif_t * WifiSTA::m_netif = nullptr;
esp_netif_ip_info_t WifiSTA::m_stGotIP = {};
int32_t WifiSTA::m_s32RetryCount = 0;

bool WifiAP::m_bStarted = false;
esp_event_handler_instance_t WifiAP::m_ins_any_id = nullptr;
esp_netif_t * WifiAP::netif = nullptr;

static void reconnect(TimerHandle_t xTimer)
{
    esp_wifi_connect();
}

static TimerHandle_t wifi_reconnect_timer = xTimerCreate("wifi_reconnect_timer", pdMS_TO_TICKS(PROJECT_WIFI_STA_RETRY_DELAY_MS), pdFALSE, NULL, &reconnect);

void wifi_sta_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        WifiSTA::m_s32RetryCount = 0;
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (++WifiSTA::m_s32RetryCount >= PROJECT_WIFI_STA_MAX_RETRY_COUNT)
        {
            ESP_LOGE(TAG_STA, "connect to the AP '%s' fail, retry count exceed", Settings::GetInstance().GetSiteSSID().c_str());
            ESP_LOGE(TAG_STA, "Wifi Station is automatically stopped!");
            WifiSTA::Stop();
            return;
        }
        xTimerStart(wifi_reconnect_timer, portMAX_DELAY);
        ESP_LOGI(TAG_STA, "connect to the AP '%s' fail, reconnect after %d second(s) ...", Settings::GetInstance().GetSiteSSID().c_str(), PROJECT_WIFI_STA_RETRY_DELAY_MS / 1000);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        WifiSTA::m_stGotIP = event->ip_info;
        ESP_LOGI(TAG_STA, "connect to the AP '%s' success", Settings::GetInstance().GetSiteSSID().c_str());
        WifiSTA::PingGateway();
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

bool WifiSTA::IsStarted()
{
    wifi_mode_t mode;
    switch (esp_wifi_get_mode(&mode))
    {
    case ESP_ERR_WIFI_NOT_INIT:
        return false;
    case ESP_ERR_INVALID_ARG:
        ESP_LOGE(TAG_STA, "Invalid argument to get wifi mode");
        return false;
    }
    return m_bStarted && m_netif != nullptr && mode == WIFI_MODE_STA;
}

esp_err_t WifiSTA::Start(bool bUseStaticIp)
{
    if (IsStarted())
    {
        ESP_LOGE(TAG_STA, "STA already started");
        return ESP_ERR_INVALID_STATE;
    }

    if (WifiAP::IsStarted())
    {
        ESP_LOGE(TAG_STA, "AP is started, cannot start STA");
        return ESP_ERR_INVALID_STATE;
    }

    m_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t stInitCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&stInitCfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL, &m_ins_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL, &m_ins_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    const std::string& sSsid = Settings::GetInstance().GetSiteSSID();
    const std::string& sPass = Settings::GetInstance().GetSitePassword();
    if (!SettingsValidator::IsValidSiteSSID(sSsid))
    {
        ESP_LOGE(TAG_STA, "Invalid Site SSID is '%s' is invalid", sSsid.c_str());
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t stCfg;
    memset(&stCfg, 0, sizeof(stCfg));
    strcpy((char *)(stCfg.sta.ssid), sSsid.c_str());
    strcpy((char *)(stCfg.sta.password), sPass.c_str());
    stCfg.sta.threshold.authmode = (sPass.length() == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK;
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &stCfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Failed to set wifi config (%s)", esp_err_to_name(err));
        return err;
    }

    if (bUseStaticIp)
    {
        const char *pIP = Settings::GetInstance().GetStaticIP().c_str();
        const char *pNetmask = Settings::GetInstance().GetNetmask().c_str();
        const char *pGW = Settings::GetInstance().GetGatewayAddress().c_str();

        ESP_LOGI(TAG_STA, "Using Static IP '%s', Netmask '%s', Gateway '%s'", pIP, pNetmask, pGW);

        esp_netif_ip_info_t ip_info;
        ip_info.ip.addr = inet_addr(pIP);
        ip_info.netmask.addr = inet_addr(pNetmask);
        ip_info.gw.addr = inet_addr(pGW);
        ESP_ERROR_CHECK(esp_netif_dhcpc_stop(m_netif));
        ESP_ERROR_CHECK(esp_netif_set_ip_info(m_netif, &ip_info));
    }

    ESP_ERROR_CHECK(esp_wifi_start());
    m_bStarted = true;

    ESP_LOGI(TAG_STA, "Wifi Station is started. SSID: '%s' password: '%s'", stCfg.sta.ssid, stCfg.sta.password);

    return ESP_OK;
}

esp_err_t WifiSTA::Stop()
{
    if (!IsStarted())
    {
        ESP_LOGE(TAG_STA, "STA not started");
        return ESP_ERR_INVALID_STATE;
    }

    if (esp_wifi_stop() != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Failed to stop STA");
    }
    if (esp_wifi_deinit() != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Failed to deinit STA");
    }
    if (esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, m_ins_any_id) != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Failed to unregister event handler");
    }
    if (esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, m_ins_got_ip) != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Failed to unregister event handler");
    }

    esp_netif_destroy_default_wifi(m_netif);
    m_netif = nullptr;

    m_bStarted = false;

    return ESP_OK;
}

esp_err_t WifiSTA::ApplyStaticIP()
{
    if (!IsStarted())
    {
        ESP_LOGE(TAG_STA, "STA not started");
        return ESP_ERR_INVALID_STATE;
    }

    const char *pIP = Settings::GetInstance().GetStaticIP().c_str();
    const char *pNetmask = Settings::GetInstance().GetNetmask().c_str();
    const char *pGW = Settings::GetInstance().GetGatewayAddress().c_str();

    esp_netif_ip_info_t ip_info;
    // Convert IP strings to ip4_addr_t using inet_addr
    ip_info.ip.addr = inet_addr(pIP);
    ip_info.netmask.addr = inet_addr(pNetmask);
    ip_info.gw.addr = inet_addr(pGW);

    // Set the static IP configuration
    ESP_LOGI(TAG_STA, "Stopping DHCP client");
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(m_netif)); // Stop DHCP client
    ESP_LOGI(TAG_STA, "Setting static IP '%s'", pIP);
    ESP_LOGI(TAG_STA, "Setting netmask '%s'", pNetmask);
    ESP_LOGI(TAG_STA, "Setting gateway '%s'", pGW);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(m_netif, &ip_info));

    m_stGotIP = ip_info;

    return ESP_OK;
}

esp_err_t WifiSTA::PingGateway()
{
    if (!IsStarted())
    {
        ESP_LOGE(TAG_STA, "STA not started");
        return ESP_ERR_INVALID_STATE;
    }

    if (m_stGotIP.gw.addr == 0)
    {
        ESP_LOGE(TAG_STA, "STA GW is unknown");
        return ESP_ERR_INVALID_STATE;
    }

    esp_ping_config_t stPingConfig = ESP_PING_DEFAULT_CONFIG();
    stPingConfig.target_addr.type = ESP_IPADDR_TYPE_V4;
    stPingConfig.target_addr.u_addr.ip4.addr = m_stGotIP.gw.addr; // Gateway address
    stPingConfig.count = 3; // Number of ping requests to send
    
    esp_ping_callbacks_t stCbs = {
        .cb_args = NULL,
        .on_ping_success = [](esp_ping_handle_t hdl, void *args) {
            uint32_t u32ElapsedTime;
            esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &u32ElapsedTime, sizeof(u32ElapsedTime));
            ESP_LOGI(TAG_STA, "Ping success, time: %ld ms", u32ElapsedTime);
        },
        .on_ping_timeout = [](esp_ping_handle_t hdl, void *args) {
            ESP_LOGW(TAG_STA, "Ping timeout");
        },
        .on_ping_end = [](esp_ping_handle_t hdl, void *args) {
            ESP_LOGI(TAG_STA, "Ping end");
            esp_ping_delete_session(hdl);
        }
    };

    esp_ping_handle_t hPing;
    esp_err_t err = esp_ping_new_session(&stPingConfig, &stCbs, &hPing);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_STA, "Failed to create ping session: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_ping_start(hPing);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_STA, "Failed to start ping: %s", esp_err_to_name(err));
        esp_ping_delete_session(hPing);
        return err;
    }

    return ESP_OK;
}

// ====================================================================================================
// WifiAP
// ====================================================================================================

esp_err_t WifiAP::Start()
{
    if (IsStarted())
    {
        ESP_LOGE(TAG_AP, "AP already started");
        return ESP_ERR_INVALID_STATE;
    }

    if (WifiSTA::IsStarted())
    {
        ESP_LOGE(TAG_AP, "STA is started, cannot start AP");
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

    esp_netif_destroy_default_wifi(netif);
    netif = nullptr;
    
    m_bStarted = false;

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