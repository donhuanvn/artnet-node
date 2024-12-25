#ifndef __ARTNET_NODE_INFO_MODEL_H__
#define __ARTNET_NODE_INFO_MODEL_H__

#include <stdio.h>
#include <string>
#include <esp_err.h>
#include "cJSON.h"
#include "nvs.h"

class InfoModel
{
    std::string m_sHostAppIP;
    nvs_handle_t m_s32NVSHandle;
public:
    static InfoModel GetInstance()
    {
        static InfoModel oIns;
        return oIns;
    }
    InfoModel();
    std::string GetMAC(const std::string& sMacType = "WIFI-STA") const;
    std::string GetIP() const;
    esp_err_t SetHostAppIP(const std::string& sHostAppIP);
    const std::string& GetHostAppIP() const { return m_sHostAppIP; }
    cJSON * ToJson();
    void Log();
};

#endif /* __ARTNET_NODE_INFO_MODEL_H__ */