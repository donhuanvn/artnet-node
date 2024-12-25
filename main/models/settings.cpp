#include "settings.h"
#include "esp_log.h"
#include "wifi.h"

static const char *TAG = "Settings-Model";

Settings::Settings()
{
    esp_err_t err;
    ESP_ERROR_CHECK(nvs_open("settings", NVS_READWRITE, &m_s32NVSHandle));
    {
        auto [ssid, pass] = WifiAP::GetInstance().GetConfig();
        m_sBroadcastSSID = ssid;
        m_sBroadcastPassword = pass;
    }
    {
        auto [ssid, pass] = WifiSTA::GetInstance().GetConfig();
        m_sSiteSSID = ssid;
        m_sSitePassword = pass;
    }
    
    char buffer[1024];
    size_t len = 0;

    err = nvs_get_str(m_s32NVSHandle, "static_ip", buffer, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sStaticIP = "";
    }
    else
    {
        m_sStaticIP.assign(buffer, len);
    }

    err = nvs_get_str(m_s32NVSHandle, "led_type", buffer, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sLedType = "";
    }
    else
    {
        m_sLedType.assign(buffer, len);
    }

    err = nvs_get_i32(m_s32NVSHandle, "time_high", &m_s32TimeHigh);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_s32TimeHigh = -1;
    }

    err = nvs_get_i32(m_s32NVSHandle, "time_low", &m_s32TimeLow);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_s32TimeLow = -1;
    }

    err = nvs_get_i32(m_s32NVSHandle, "start_universe", &m_s32StartUniverse);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_s32StartUniverse = -1;
    }

    err = nvs_get_i32(m_s32NVSHandle, "no_universes", &m_s32NoUniverses);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_s32NoUniverses = -1;
    }

    err = nvs_get_str(m_s32NVSHandle, "identity", buffer, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sIdentity = "";
    }
    else
    {
        m_sIdentity.assign(buffer, len);
    }

    err = nvs_get_str(m_s32NVSHandle, "model", buffer, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sModel = "";
    }
    else
    {
        m_sModel.assign(buffer, len);
    }

    err = nvs_get_str(m_s32NVSHandle, "product_id", buffer, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sProductID = "";
    }
    else
    {
        m_sProductID.assign(buffer, len);
    }

    uint8_t bEnabled = 0; 
    err = nvs_get_u8(m_s32NVSHandle, "artnet_sync", &bEnabled);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_bArtNetSyncEnabled = false;
    }
    else
    {
        m_bArtNetSyncEnabled = (bool)bEnabled;
    }
}

void Settings::FromJson(const cJSON *json)
{
    cJSON * pItem = NULL;

    pItem = cJSON_GetObjectItemCaseSensitive(json, "BroadcastSSID");
    if (cJSON_IsString(pItem))
    {
        SetBroadcastSSID(pItem->valuestring);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "BroadcastPassword");
    if (cJSON_IsString(pItem))
    {
        SetBroadcastPassword(pItem->valuestring);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "SiteSSID");
    if (cJSON_IsString(pItem))
    {
        SetSiteSSID(pItem->valuestring);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "SitePassword");
    if (cJSON_IsString(pItem))
    {
        SetSitePassword(pItem->valuestring);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "StaticIP");
    if (cJSON_IsString(pItem))
    {
        SetStaticIP(pItem->valuestring);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "LedType");
    if (cJSON_IsString(pItem))
    {
        SetLedType(pItem->valuestring);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "TimeHigh");
    if (cJSON_IsNumber(pItem))
    {
        SetTimeHigh(pItem->valueint);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "TimeLow");
    if (cJSON_IsNumber(pItem))
    {
        SetTimeLow(pItem->valueint);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "StartUniverse");
    if (cJSON_IsNumber(pItem))
    {
        SetStartUniverse(pItem->valueint);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "NoUniverses");
    if (cJSON_IsNumber(pItem))
    {
        SetNoUniverses(pItem->valueint);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "Identity");
    if (cJSON_IsString(pItem))
    {
        SetIdentity(pItem->valuestring);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "Model");
    if (cJSON_IsString(pItem))
    {
        SetModel(pItem->valuestring);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "ProductID");
    if (cJSON_IsString(pItem))
    {
        SetProductID(pItem->valuestring);
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "ArtNetSync");
    if (cJSON_IsBool(pItem))
    {
        SetArtNetSyncEnabled(cJSON_IsTrue(pItem));
    }
}

cJSON *Settings::ToJson()
{
    cJSON * pJson = cJSON_CreateObject();

    cJSON_AddStringToObject(pJson, "BroadcastSSID", m_sBroadcastSSID.c_str());
    cJSON_AddStringToObject(pJson, "BroadcastPassword", m_sBroadcastPassword.c_str());
    cJSON_AddStringToObject(pJson, "SiteSSID", m_sSiteSSID.c_str());
    cJSON_AddStringToObject(pJson, "SitePassword", m_sSitePassword.c_str());
    cJSON_AddStringToObject(pJson, "StaticIP", m_sStaticIP.c_str());
    cJSON_AddStringToObject(pJson, "LedType", m_sLedType.c_str());
    cJSON_AddNumberToObject(pJson, "TimeHigh", m_s32TimeHigh);
    cJSON_AddNumberToObject(pJson, "TimeLow", m_s32TimeLow);
    cJSON_AddNumberToObject(pJson, "StartUniverse", m_s32StartUniverse);
    cJSON_AddNumberToObject(pJson, "NoUniverses", m_s32NoUniverses);
    cJSON_AddStringToObject(pJson, "Identity", m_sIdentity.c_str());
    cJSON_AddStringToObject(pJson, "Model", m_sModel.c_str());
    cJSON_AddStringToObject(pJson, "ProductID", m_sProductID.c_str());
    cJSON_AddBoolToObject(pJson, "ArtNetSync", m_bArtNetSyncEnabled);

    return pJson;
}

void Settings::Log()
{
    cJSON * json = ToJson();
    ESP_LOGI(TAG, "Settings: \n%s", cJSON_Print(json));
    cJSON_Delete(json);
}

esp_err_t Settings::SetBroadcastSSID(const std::string &sSsid)
{
    WifiAP::GetInstance().Config(sSsid, m_sBroadcastPassword);
    m_sBroadcastSSID = sSsid;
    return ESP_OK;
}

esp_err_t Settings::SetBroadcastPassword(const std::string &sPassword)
{
    WifiAP::GetInstance().Config(m_sBroadcastSSID, sPassword);
    m_sBroadcastPassword = sPassword;
    return ESP_OK;
}

esp_err_t Settings::SetSiteSSID(const std::string &sSsid)
{
    WifiSTA::GetInstance().Config(sSsid, m_sSitePassword);
    m_sSiteSSID = sSsid;
    return ESP_OK;
}

esp_err_t Settings::SetSitePassword(const std::string &sPassword)
{
    WifiSTA::GetInstance().Config(m_sSiteSSID, sPassword);
    m_sSitePassword = sPassword;
    return ESP_OK;
}

esp_err_t Settings::SetStaticIP(const std::string &sIP)
{
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "static_ip", sIP.c_str()));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_sStaticIP = sIP;
    }
    return err;
}

esp_err_t Settings::SetLedType(const std::string &sType)
{
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "led_type", sType.c_str()));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_sLedType = sType;
    }
    return err;
}

esp_err_t Settings::SetTimeHigh(int32_t s32Time)
{
    ESP_ERROR_CHECK(nvs_set_i32(m_s32NVSHandle, "time_high", s32Time));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_s32TimeHigh = s32Time;
    }
    return err;
}

esp_err_t Settings::SetTimeLow(int32_t s32Time)
{
    ESP_ERROR_CHECK(nvs_set_i32(m_s32NVSHandle, "time_low", s32Time));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_s32TimeLow = s32Time;
    }
    return err;
}

esp_err_t Settings::SetStartUniverse(int32_t s32Univ)
{
    ESP_ERROR_CHECK(nvs_set_i32(m_s32NVSHandle, "start_universe", s32Univ));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_s32StartUniverse = s32Univ;
    }
    return err;
}

esp_err_t Settings::SetNoUniverses(int32_t s32Count)
{
    ESP_ERROR_CHECK(nvs_set_i32(m_s32NVSHandle, "no_universes", s32Count));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_s32NoUniverses = s32Count;
    }
    return err;
}

esp_err_t Settings::SetIdentity(const std::string &sIdentity)
{
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "identity", sIdentity.c_str()));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_sIdentity = sIdentity;
    }
    return err;
}

esp_err_t Settings::SetModel(const std::string &sModel)
{
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "model", sModel.c_str()));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_sModel = sModel;
    }
    return err;
}

esp_err_t Settings::SetProductID(const std::string &sProductID)
{
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "product_id", sProductID.c_str()));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_sProductID = sProductID;
    }
    return err;
}

esp_err_t Settings::SetArtNetSyncEnabled(bool bEnabled)
{
    ESP_ERROR_CHECK(nvs_set_u8(m_s32NVSHandle, "artnet_sync", (uint8_t)bEnabled));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_bArtNetSyncEnabled = bEnabled;
    }
    return err;
}
