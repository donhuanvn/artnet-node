#ifndef __WIFI_H__
#define __WIFI_H__

#include <string>
#include <utility>
#include "esp_system.h"
#include "esp_wifi.h"

class WifiSTA
{
    bool m_bInit;
    esp_event_handler_instance_t m_ins_any_id;
    esp_event_handler_instance_t m_ins_got_ip;
    esp_netif_t * m_netif;
    std::string m_sGotIP;
    std::string m_sGotNetmask;
    std::string m_sGatewayAddress;
    friend void wifi_sta_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
public:
    static WifiSTA& GetInstance()
    {
        static WifiSTA oIns;
        return oIns;
    }
    void Init();
    bool HasInit();
    void Deinit();
    void Config(const std::string &s_ssid, const std::string &s_password);
    bool HasValidConfig();
    std::pair<std::string, std::string> GetConfig();
    std::string GetIP() { return m_sGotIP; }
    std::string GetNetmask() { return m_sGotNetmask; }
    std::string GetGatewayAddress() { return m_sGatewayAddress; }
};

class WifiAP
{
    bool m_bInit;
    esp_event_handler_instance_t m_ins_any_id;
    esp_netif_t * netif;
public:
    static WifiAP& GetInstance()
    {
        static WifiAP oIns;
        return oIns;
    }
    void Init();
    bool HasInit();
    void Deinit();
    void Config(const std::string &s_ssid, const std::string &s_password);
    bool HasValidConfig();
    std::pair<std::string, std::string> GetConfig();
};

#endif /* __WIFI_H__ */