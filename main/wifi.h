#ifndef __WIFI_H__
#define __WIFI_H__

#include <string>
#include "esp_system.h"
#include "esp_wifi.h"

class WifiSTA
{
    bool bInit;
    esp_event_handler_instance_t ins_any_id;
    esp_event_handler_instance_t ins_got_ip;
    esp_netif_t * netif;
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
};

class WifiAP
{
    bool bInit;
    esp_event_handler_instance_t ins_any_id;
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
};

#endif /* __WIFI_H__ */