#ifndef __WIFI_H__
#define __WIFI_H__

#include <string>
#include <functional>
#include "esp_system.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "freertos/event_groups.h"

class WifiSTA
{
public:
    enum class State
    {
        STOPPED,
        STARTED,
        WIFI_CONNECTED,
        WIFI_DISCONNECTED,
        IP_CONNECTED,
        IP_DISCONNECTED,
    };

    enum class Event
    {

    };

private:
    static State m_eState;
    static std::function<void(State,State)> m_fnStateChangeCallback;
    static EventGroupHandle_t m_stWifiEventGroup;
    static const int m_STOPPED_BIT = BIT0;
    static const int m_STARTED_BIT = BIT1;
    static const int m_WIFI_CONNECTED_BIT = BIT2;
    static const int m_WIFI_DISCONNECTED_BIT = BIT3;
    static const int m_IP_CONNECTED_BIT = BIT4;
    static const int m_IP_DISCONNECTED_BIT = BIT5;

    static bool m_bUseStaticIp;
    static esp_event_handler_instance_t m_ins_any_id;
    static esp_event_handler_instance_t m_ins_got_ip;
    static esp_netif_t * m_netif;
    static esp_netif_ip_info_t m_stGotIP;
    static int32_t m_s32RetryCount;

    static TimerHandle_t m_hReconnectTimer;
    static void Reconnect(TimerHandle_t xTimer);
    friend void wifi_sta_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void SetState(State eState);
    static esp_err_t PingGateway();

public:
    static State GetState() { return m_eState; }
    static void SetStateChangeCallback(std::function<void(State,State)> fn) { m_fnStateChangeCallback = fn; }

    static esp_err_t Start(bool bBlocking = false);
    static bool IsStarted();
    static esp_err_t Stop();
    static bool IsUseStaticIp() { return m_bUseStaticIp; }
    static esp_err_t ApplyStaticIP();
    static esp_err_t CheckStaticIP(bool bBlocking = false);
    static esp_err_t ApplyDHCP();

    static std::string GetIP() { return inet_ntoa(m_stGotIP.ip); }
    static std::string GetNetmask() { return inet_ntoa(m_stGotIP.netmask); }
    static std::string GetGatewayAddress() { return inet_ntoa(m_stGotIP.gw); }
};

class WifiAP
{
public:
    static bool m_bStarted;
    static esp_event_handler_instance_t m_ins_any_id;
    static esp_netif_t * netif;
public:
    static esp_err_t Start();
    static bool IsStarted();
    static esp_err_t Stop();
};

class WifiAutoConnect
{
    static TaskHandle_t m_hTask;
    static int32_t m_s32TimeCountMs;
    static void FreeRTOSTask(void *pvParameters);
public:
    static esp_err_t Start();
    static esp_err_t Stop();
};

#endif /* __WIFI_H__ */