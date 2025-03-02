#ifndef __WIFI_H__
#define __WIFI_H__

#include <string>
#include <utility>
#include "esp_system.h"
#include "esp_wifi.h"
#include "lwip/inet.h"

class WifiSTA
{
    static bool m_bStarted;
    static esp_event_handler_instance_t m_ins_any_id;
    static esp_event_handler_instance_t m_ins_got_ip;
    static esp_netif_t * m_netif;
    static esp_netif_ip_info_t m_stGotIP;
    static int32_t m_s32RetryCount;
    friend void wifi_sta_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
public:
    static esp_err_t Start(bool bUseStaticIp = false);
    static bool IsStarted();
    static esp_err_t Stop();
    static esp_err_t ApplyStaticIP();
    static esp_err_t PingGateway();

    static std::string GetIP() { return inet_ntoa(m_stGotIP.ip); }
    static std::string GetNetmask() { return inet_ntoa(m_stGotIP.netmask); }
    static std::string GetGatewayAddress() { return inet_ntoa(m_stGotIP.gw); }
};

class WifiAP
{
    static bool m_bStarted;
    static esp_event_handler_instance_t m_ins_any_id;
    static esp_netif_t * netif;
public:
    static esp_err_t Start();
    static bool IsStarted();
    static esp_err_t Stop();
    static int32_t GetClientsCount();
};

#endif /* __WIFI_H__ */