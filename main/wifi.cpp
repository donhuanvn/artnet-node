#include "wifi.h"
#include "config.h"
#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_mac.h"
#include "models/settings.h"
#include "esp_netif.h"
#include "ping/ping_sock.h"

#ifndef PROJECT_WIFI_STA_RETRY_DELAY_MS
#define PROJECT_WIFI_STA_RETRY_DELAY_MS 3000
#endif

#ifndef PROJECT_WIFI_STA_CHECK_STATIC_IP_PERIOD_MS
#define PROJECT_WIFI_STA_CHECK_STATIC_IP_PERIOD_MS 60000
#endif

#ifndef PROJECT_WIFI_STA_DHCP_LIVE_DURATION_MS
#define PROJECT_WIFI_STA_DHCP_LIVE_DURATION_MS 180000
#endif

#ifndef PROJECT_WIFI_AP_LIVE_DURATION_MS
#define PROJECT_WIFI_AP_LIVE_DURATION_MS 90000
#endif

static const char *TAG_STA = "WiFi-STA";
static const char *TAG_AP = "WiFi-AP";
static const char *TAG_AUTO_CONNECT = "WiFi-AutoConnect";

bool WifiSTA::m_bUseStaticIp = false;
WifiSTA::State WifiSTA::m_eState = WifiSTA::State::STOPPED;
std::function<void(WifiSTA::State,WifiSTA::State)> WifiSTA::m_fnStateChangeCallback = nullptr;
esp_event_handler_instance_t WifiSTA::m_ins_any_id = nullptr;
esp_event_handler_instance_t WifiSTA::m_ins_got_ip = nullptr;
esp_netif_t * WifiSTA::m_netif = nullptr;
esp_netif_ip_info_t WifiSTA::m_stGotIP = {};
int32_t WifiSTA::m_s32RetryCount = 0;
EventGroupHandle_t WifiSTA::m_stWifiEventGroup = xEventGroupCreate();
TimerHandle_t WifiSTA::m_hReconnectTimer = xTimerCreate("WifiSTA::m_hReconnectTimer", pdMS_TO_TICKS(PROJECT_WIFI_STA_RETRY_DELAY_MS), pdFALSE, NULL, &WifiSTA::Reconnect);
bool WifiAP::m_bStarted = false;
esp_event_handler_instance_t WifiAP::m_ins_any_id = nullptr;
esp_netif_t * WifiAP::netif = nullptr;

TaskHandle_t WifiAutoConnect::m_hTask = nullptr;

void wifi_sta_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        WifiSTA::m_s32RetryCount = 0;
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG_STA, "connect to the AP '%s' success", Settings::GetInstance().GetSiteSSID().c_str());
        WifiSTA::SetState(WifiSTA::State::WIFI_CONNECTED);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (++WifiSTA::m_s32RetryCount >= PROJECT_WIFI_STA_MAX_RETRY_COUNT)
        {
            ESP_LOGE(TAG_STA, "connect to the AP '%s' fail, retry count exceed", Settings::GetInstance().GetSiteSSID().c_str());
            WifiSTA::SetState(WifiSTA::State::WIFI_DISCONNECTED);
            return;
        }
        xTimerStart(WifiSTA::m_hReconnectTimer, portMAX_DELAY);
        ESP_LOGI(TAG_STA, "connect to the AP '%s' fail, reconnect after %d second(s) ...", Settings::GetInstance().GetSiteSSID().c_str(), PROJECT_WIFI_STA_RETRY_DELAY_MS / 1000);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        WifiSTA::m_stGotIP = event->ip_info;
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

void WifiSTA::Reconnect(TimerHandle_t xTimer)
{
    esp_wifi_connect();
}

void WifiSTA::SetState(State eState)
{
    switch (eState)
    {
    case State::STOPPED:
        xEventGroupSetBits(WifiSTA::m_stWifiEventGroup, WifiSTA::m_STOPPED_BIT);
        break;
    case State::STARTED:
        xEventGroupSetBits(WifiSTA::m_stWifiEventGroup, WifiSTA::m_STARTED_BIT);
        break;
    case State::WIFI_CONNECTED:
        xEventGroupSetBits(WifiSTA::m_stWifiEventGroup, WifiSTA::m_WIFI_CONNECTED_BIT);
        break;
    case State::WIFI_DISCONNECTED:
        xEventGroupSetBits(WifiSTA::m_stWifiEventGroup, WifiSTA::m_WIFI_DISCONNECTED_BIT);
        break;
    case State::IP_CONNECTED:
        xEventGroupSetBits(WifiSTA::m_stWifiEventGroup, WifiSTA::m_IP_CONNECTED_BIT);
        break;
    case State::IP_DISCONNECTED:
        xEventGroupSetBits(WifiSTA::m_stWifiEventGroup, WifiSTA::m_IP_DISCONNECTED_BIT);
        break;
    default:
        break;
    }

    State eStateOld = m_eState;
    m_eState = eState;
    if (m_fnStateChangeCallback)
    {
        m_fnStateChangeCallback(eState, eStateOld);
    }
}

bool WifiSTA::IsStarted()
{
    return (m_eState != State::STOPPED);
}

esp_err_t WifiSTA::Start(bool bBlocking)
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

    // Disable DHCP client
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(m_netif));

    ESP_ERROR_CHECK(esp_wifi_start());

    SetState(State::STARTED);

    ESP_LOGI(TAG_STA, "Wifi Station is started. SSID: '%s' password: '%s'", stCfg.sta.ssid, stCfg.sta.password);

    if (bBlocking)
    {
        // Block until wifi connected or failed.
        xEventGroupWaitBits(m_stWifiEventGroup, m_WIFI_CONNECTED_BIT | m_WIFI_DISCONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    }

    // Alway return ESP_OK. Use GetState() to check if wifi connected or not.
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

    SetState(State::STOPPED);

    return ESP_OK;
}

esp_err_t WifiSTA::ApplyDHCP()
{
    if (!IsStarted())
    {
        ESP_LOGE(TAG_STA, "STA not started");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = esp_netif_dhcpc_start(m_netif);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Failed to start DHCP client: %s", esp_err_to_name(err));
        return err;
    }

    m_bUseStaticIp = false;
    return ESP_OK;
}

esp_err_t WifiSTA::ApplyStaticIP()
{
    if (!IsStarted())
    {
        ESP_LOGE(TAG_STA, "STA not started");
        return ESP_ERR_INVALID_STATE;
    }

    SetState(State::WIFI_CONNECTED); // Downgrade state to WIFI_CONNECTED

    const char *pIP = Settings::GetInstance().GetStaticIP().c_str();
    const char *pNetmask = Settings::GetInstance().GetNetmask().c_str();
    const char *pGW = Settings::GetInstance().GetGatewayAddress().c_str();

    esp_netif_ip_info_t ip_info;
    // Convert IP strings to ip4_addr_t using inet_addr
    ip_info.ip.addr = inet_addr(pIP);
    ip_info.netmask.addr = inet_addr(pNetmask);
    ip_info.gw.addr = inet_addr(pGW);

    // Set the static IP configuration
    esp_netif_dhcpc_stop(m_netif);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(m_netif, &ip_info));

    m_stGotIP = ip_info;
    m_bUseStaticIp = true;

    return ESP_OK;
}

esp_err_t WifiSTA::CheckStaticIP(bool bBlocking)
{
    if (WifiSTA::PingGateway() != ESP_OK)
    {
        ESP_LOGE(TAG_STA, "Failed to ping gateway");
        return ESP_FAIL;
    }

    if (bBlocking)
    {
        // Block until checking IP completed (both success or failure).
        xEventGroupWaitBits(m_stWifiEventGroup, m_IP_CONNECTED_BIT | m_IP_DISCONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    }

    // Alway return ESP_OK. Use GetState() to check if IP connected or not.
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
    stPingConfig.target_addr = IPADDR4_INIT(m_stGotIP.gw.addr);
    stPingConfig.count = 3;

    static bool bPingHasSuccess;
    bPingHasSuccess = false;
    esp_ping_callbacks_t stCbs = {
        .cb_args = (void*)(&bPingHasSuccess),
        .on_ping_success = [](esp_ping_handle_t hdl, void *args) {
            uint32_t u32ElapsedTime;
            esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &u32ElapsedTime, sizeof(u32ElapsedTime));
            ESP_LOGI(TAG_STA, "Ping success, RTT: %ld ms", u32ElapsedTime);
            SetState(State::IP_CONNECTED);
            *(bool*)(args) = true;
            esp_ping_stop(hdl);
        },
        .on_ping_timeout = [](esp_ping_handle_t hdl, void *args) {
            ESP_LOGW(TAG_STA, "Ping timeout");
        },
        .on_ping_end = [](esp_ping_handle_t hdl, void *args) {
            if (!*(bool*)(args) && (GetState() == State::IP_CONNECTED || GetState() == State::WIFI_CONNECTED))
            {
                SetState(State::IP_DISCONNECTED);
            }
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

// ====================================================================================================
// WifiAutoConnect
// ====================================================================================================

void WifiAutoConnect::FreeRTOSTask(void *pvParameters)
{
    ESP_LOGI(TAG_AUTO_CONNECT, "FreeRTOSTask Task starts");

    while (true)
    {
        // If WiFi is not started in any mode, try to start WiFi STA first.
        // If Wifi STA is connected, apply and check static IP.
        // If static IP is not workable, apply DHCP and retry static IP after 3 minutes.
        // If static IP is workable, delay 1 minute and then check the IP again and again.
        // When Wifi STA is disconnected, stop STA and start AP mode.
        // If WiFi AP is started, delay 1.5 minutes and then try to start WiFi STA mode.

        if (!WifiSTA::IsStarted() && !WifiAP::IsStarted())
        {
            WifiSTA::Start(true);
        }
        else if (WifiSTA::IsStarted())
        {
            switch (WifiSTA::GetState())
            {
            case WifiSTA::State::IP_CONNECTED:
            {
                vTaskDelay(pdMS_TO_TICKS(PROJECT_WIFI_STA_CHECK_STATIC_IP_PERIOD_MS));
                WifiSTA::CheckStaticIP(true);
            }
            break;
            case WifiSTA::State::IP_DISCONNECTED:
            {
                ESP_LOGI(TAG_AUTO_CONNECT, "Applying DHCP");
                WifiSTA::ApplyDHCP();
                vTaskDelay(pdMS_TO_TICKS(PROJECT_WIFI_STA_DHCP_LIVE_DURATION_MS));
                ESP_LOGI(TAG_AUTO_CONNECT, "Re-try Static IP");
                WifiSTA::ApplyStaticIP();
                WifiSTA::CheckStaticIP(true);
            }
            break;
            case WifiSTA::State::WIFI_CONNECTED:
            {
                ESP_LOGI(TAG_AUTO_CONNECT, "Applying static IP");
                WifiSTA::ApplyStaticIP();
                WifiSTA::CheckStaticIP(true);
            }
            break;
            case WifiSTA::State::WIFI_DISCONNECTED:
            {
                ESP_LOGI(TAG_AUTO_CONNECT, "WiFi STA disconnected, starting AP mode");
                WifiSTA::Stop();
                WifiAP::Start();
            }
            break;
            case WifiSTA::State::STARTED:
            {
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            break;
            default:
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            break;
            }
        }
        else if (WifiAP::IsStarted())
        {
            ESP_LOGI(TAG_AUTO_CONNECT, "WiFi AP mode started, delaying %.1f minute(s)", (float)(PROJECT_WIFI_AP_LIVE_DURATION_MS) / 60000);
            vTaskDelay(pdMS_TO_TICKS(PROJECT_WIFI_AP_LIVE_DURATION_MS));
            ESP_LOGI(TAG_AUTO_CONNECT, "Stopping AP mode and re-try STA mode");
            WifiAP::Stop();
            WifiSTA::Start(true);
        }
    }
}

esp_err_t WifiAutoConnect::Start()
{
    if (WifiSTA::IsStarted() || WifiAP::IsStarted())
    {
        ESP_LOGE(TAG_AUTO_CONNECT, "Wifi is already started");
        return ESP_ERR_INVALID_STATE;
    }

    if (m_hTask != nullptr)
    {
        ESP_LOGE(TAG_AUTO_CONNECT, "Task already started");
        return ESP_ERR_INVALID_STATE;
    }

    xTaskCreate(FreeRTOSTask, "WifiAutoConnect::FreeRTOSTask", 4096, NULL, configMAX_PRIORITIES - 2, &m_hTask);
    return ESP_OK;
}

esp_err_t WifiAutoConnect::Stop()
{
    if (m_hTask == nullptr)
    {
        ESP_LOGE(TAG_AUTO_CONNECT, "Task not started");
        return ESP_ERR_INVALID_STATE;
    }

    vTaskDelete(m_hTask);
    m_hTask = nullptr;

    ESP_LOGI(TAG_AUTO_CONNECT, "Task stopped");
    return ESP_OK;
}
