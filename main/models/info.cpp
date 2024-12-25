#include "info.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "wifi.h"

static const char *TAG = "Info-Model";

InfoModel::InfoModel()
{
    ESP_ERROR_CHECK(nvs_open("info", NVS_READWRITE, &m_s32NVSHandle));
}

cJSON * InfoModel::ToJson()
{
    cJSON * json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "MAC", GetMAC().c_str());
    cJSON_AddStringToObject(json, "AssignedIP", GetIP().c_str());
    cJSON_AddStringToObject(json, "HostAppIP", GetHostAppIP().c_str());
    return json;
}

void InfoModel::Log()
{
    cJSON * json = ToJson();
    ESP_LOGI(TAG, "Info: \n%s", cJSON_Print(json));
    cJSON_Delete(json);
}

std::string InfoModel::GetMAC(const std::string& sMacType) const 
{
    uint8_t au8Mac[6] = {0};
    if (sMacType == "WIFI-STA")
    {
        ESP_ERROR_CHECK(esp_read_mac(au8Mac, ESP_MAC_WIFI_STA));
    }
    else if (sMacType == "WIFI-AP")
    {
        ESP_ERROR_CHECK(esp_read_mac(au8Mac, ESP_MAC_WIFI_SOFTAP));
    }
    else
    {
        ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
    }

    std::string sMAC(17, ':');
    sprintf(sMAC.data(), "%02x:%02x:%02x:%02x:%02x:%02x", au8Mac[0], au8Mac[1], au8Mac[2], au8Mac[3], au8Mac[4], au8Mac[5]);

    return sMAC;
}

std::string InfoModel::GetIP() const 
{
    return WifiSTA::GetInstance().GetIP();
}

esp_err_t InfoModel::SetHostAppIP(const std::string& sHostAppIP)
{
    esp_err_t err = nvs_set_str(m_s32NVSHandle, "host_app_ip", sHostAppIP.c_str());
    if (err == ESP_OK)
    {
        m_sHostAppIP = sHostAppIP;
    }
    return err;
}