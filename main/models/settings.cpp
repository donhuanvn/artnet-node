#include "settings.h"
#include "esp_log.h"
#include "wifi.h"
#include "string.h"
#include "miscellaneous.h"

#define BUFFER_LENGTH 1024

static const char *TAG = "Settings-Model";

#define DEFAULT_SETTING_BROADCAST_SSID "ESP32_AP"
#define DEFAULT_SETTING_TIME_HIGH 1000
#define DEFAULT_SETTING_TIME_LOW 200
#define DEFAULT_SETTING_START_UNIVERSE 0
#define DEFAULT_SETTING_NO_UNIVERSES 1
#define DEFAULT_SETTING_IDENTITY "ARTNET_NODE"
#define DEFAULT_SETTING_MODEL "ARTNET_ESP32_WIFI2.4GHZ"
#define DEFAULT_LED_TYPE "LED1903"

bool SettingsValidator::IsValidTimeHigh(int32_t s32TimeHigh)
{
    return (200 <= s32TimeHigh) && (s32TimeHigh <= 1000);
}

bool SettingsValidator::IsValidTimeLow(int32_t s32TimeLow)
{
    return (200 <= s32TimeLow) && (s32TimeLow <= 1000);
}

bool SettingsValidator::IsValidIdentity(const std::string& sIdentity)
{
    return sIdentity.length() > 0;
}

bool SettingsValidator::IsValidModel(const std::string& sModel)
{
    return sModel.length() > 0;
}

bool SettingsValidator::IsValidLedType(const std::string& sLedType)
{
    return Existing::LedTypeOnline::IsValidLedTypeString(sLedType);
}

Settings::Settings()
{
    esp_err_t err;
    ESP_ERROR_CHECK(nvs_open("settings", NVS_READWRITE, &m_s32NVSHandle));
    char buffer[BUFFER_LENGTH];
    size_t len = BUFFER_LENGTH;

    err = nvs_get_str(m_s32NVSHandle, "bcast_ssid", buffer, &len);
    if (err == ESP_OK)
    {
        m_sBroadcastSSID.assign(buffer, len - 1); // exclude null character.
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sBroadcastSSID = "";
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    if (m_sBroadcastSSID.empty())
    {
        m_sBroadcastSSID = DEFAULT_SETTING_BROADCAST_SSID;
    }

    len = BUFFER_LENGTH;
    err = nvs_get_str(m_s32NVSHandle, "bcast_pass", buffer, &len);
    if (err == ESP_OK)
    {
        m_sBroadcastPassword.assign(buffer, len - 1); // exclude null character.
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sBroadcastPassword = "";
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    len = BUFFER_LENGTH;
    err = nvs_get_str(m_s32NVSHandle, "site_ssid", buffer, &len);
    if (err == ESP_OK)
    {
        m_sSiteSSID.assign(buffer, len - 1); // exclude null character.
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sSiteSSID = "";
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    len = BUFFER_LENGTH;
    err = nvs_get_str(m_s32NVSHandle, "site_password", buffer, &len);
    if (err == ESP_OK)
    {
        m_sSitePassword.assign(buffer, len - 1); // exclude null character.
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sSitePassword = "";
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    len = BUFFER_LENGTH;
    err = nvs_get_str(m_s32NVSHandle, "static_ip", buffer, &len);
    if (err == ESP_OK)
    {
        m_sStaticIP.assign(buffer, len - 1); // exclude null character.
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sStaticIP = "";
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    len = BUFFER_LENGTH;
    err = nvs_get_str(m_s32NVSHandle, "netmask", buffer, &len);
    if (err == ESP_OK)
    {
        m_sNetmask.assign(buffer, len - 1); // exclude null character.
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sNetmask = "";
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }
    
    len = BUFFER_LENGTH;
    err = nvs_get_str(m_s32NVSHandle, "gw_addr", buffer, &len);
    if (err == ESP_OK)
    {
        m_sGatewayAddress.assign(buffer, len - 1); // exclude null character.
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sGatewayAddress = "";
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    len = BUFFER_LENGTH;
    err = nvs_get_str(m_s32NVSHandle, "led_type", buffer, &len);
    if (err == ESP_OK)
    {
        m_sLedType.assign(buffer, len - 1); // exclude null character.
        if (!SettingsValidator::IsValidLedType(m_sLedType))
        {
            m_sLedType = DEFAULT_LED_TYPE;
        }
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sLedType = DEFAULT_LED_TYPE;
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    err = nvs_get_i32(m_s32NVSHandle, "time_high", &m_s32TimeHigh);
    if (err == ESP_ERR_NVS_NOT_FOUND || !SettingsValidator::IsValidTimeHigh(m_s32TimeHigh))
    {
        m_s32TimeHigh = DEFAULT_SETTING_TIME_HIGH;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    err = nvs_get_i32(m_s32NVSHandle, "time_low", &m_s32TimeLow);
    if (err == ESP_ERR_NVS_NOT_FOUND || !SettingsValidator::IsValidTimeLow(m_s32TimeLow))
    {
        m_s32TimeLow = DEFAULT_SETTING_TIME_LOW;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }
    

    err = nvs_get_i32(m_s32NVSHandle, "start_universe", &m_s32StartUniverse);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_s32StartUniverse = DEFAULT_SETTING_START_UNIVERSE;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    err = nvs_get_i32(m_s32NVSHandle, "no_universes", &m_s32NoUniverses);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_s32NoUniverses = DEFAULT_SETTING_NO_UNIVERSES;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    len = BUFFER_LENGTH;
    err = nvs_get_str(m_s32NVSHandle, "identity", buffer, &len);
    if (err == ESP_OK)
    {
        m_sIdentity.assign(buffer, len - 1);
        if (!SettingsValidator::IsValidIdentity(m_sIdentity))
        {
            m_sIdentity = DEFAULT_SETTING_IDENTITY;
        }
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sIdentity = DEFAULT_SETTING_IDENTITY;
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    len = BUFFER_LENGTH;
    err = nvs_get_str(m_s32NVSHandle, "model", buffer, &len);
    if (err == ESP_OK)
    {
        m_sModel.assign(buffer, len - 1); // exclude null character.
        if (!SettingsValidator::IsValidModel(m_sModel))
        {
            m_sModel = DEFAULT_SETTING_MODEL;
        }
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sModel = DEFAULT_SETTING_MODEL;
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    len = BUFFER_LENGTH;
    err = nvs_get_str(m_s32NVSHandle, "product_id", buffer, &len);
    if (err == ESP_OK)
    {
        m_sProductID.assign(buffer, len - 1); // exclude null character.
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_sProductID = "";
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    uint8_t bEnabled = 0; 
    err = nvs_get_u8(m_s32NVSHandle, "artnet_sync", &bEnabled);
    if (err == ESP_OK)
    {
        m_bArtNetSyncEnabled = (bool)bEnabled;
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_bArtNetSyncEnabled = false;
    }
    else
    {
        ESP_LOGE(TAG, "Access NVS Error %s", esp_err_to_name(err));
    }

    len = BUFFER_LENGTH;
    err = nvs_get_str(m_s32NVSHandle, "ports", buffer, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        m_aPorts.fill(PortSettings());
    }
    else if (err == ESP_OK)
    {
        cJSON * json = cJSON_Parse(buffer);
        if (!cJSON_IsArray(json) || cJSON_GetArraySize(json) != PROJECT_NUMBER_OF_PORTS)
        {
            m_aPorts.fill(PortSettings());
        }
        else
        {
            // index: 0-based port number, item: port number.
            for (int32_t i=0; i < PROJECT_NUMBER_OF_PORTS; ++i)
            {
                cJSON * pPort = cJSON_GetArrayItem(json, i);
                {
                    cJSON * pItem = cJSON_GetObjectItemCaseSensitive(pPort, "StartUniverse");
                    if (pItem != NULL && cJSON_IsNumber(pItem))
                    {
                        m_aPorts[i].m_s32StartUniverse = cJSON_GetNumberValue(pItem);
                    }
                }
                {
                    cJSON * pItem = cJSON_GetObjectItemCaseSensitive(pPort, "NoUniverses");
                    if (pItem != NULL && cJSON_IsNumber(pItem))
                    {
                        m_aPorts[i].m_s32NoUniverses = cJSON_GetNumberValue(pItem);
                    }
                }
                {
                    cJSON * pItem = cJSON_GetObjectItemCaseSensitive(pPort, "LedCount");
                    if (pItem != NULL && cJSON_IsNumber(pItem))
                    {
                        m_aPorts[i].m_s32LedCount = cJSON_GetNumberValue(pItem);
                    }
                }
                {
                    cJSON * pItem = cJSON_GetObjectItemCaseSensitive(pPort, "LedType");
                    if (pItem != NULL && cJSON_IsString(pItem))
                    {
                        m_aPorts[i].m_sLedType = cJSON_GetStringValue(pItem);
                    }
                }
            }
        }
        cJSON_Delete(json);
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

    pItem = cJSON_GetObjectItemCaseSensitive(json, "Netmask");
    if (cJSON_IsString(pItem))
    {
        SetNetmask(pItem->valuestring);
    }
    
    pItem = cJSON_GetObjectItemCaseSensitive(json, "GatewayAddress");
    if (cJSON_IsString(pItem))
    {
        SetGatewayAddress(pItem->valuestring);
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

    pItem = cJSON_GetObjectItemCaseSensitive(json, "Ports");
    if (cJSON_IsArray(pItem))
    {
        // 0-based index
        for (int32_t i=0; i<cJSON_GetArraySize(pItem); ++i)
        {
            cJSON *pPort = cJSON_GetArrayItem(pItem, i);
            {
                cJSON *pPortItem = cJSON_GetObjectItemCaseSensitive(pPort, "StartUniverse");
                if (cJSON_IsNumber(pPortItem))
                {
                    m_aPorts[i].m_s32StartUniverse = cJSON_GetNumberValue(pPortItem);
                }
            }
            {
                cJSON *pPortItem = cJSON_GetObjectItemCaseSensitive(pPort, "NoUniverses");
                if (cJSON_IsNumber(pPortItem))
                {
                    m_aPorts[i].m_s32NoUniverses = cJSON_GetNumberValue(pPortItem);
                }
            }
            {
                cJSON *pPortItem = cJSON_GetObjectItemCaseSensitive(pPort, "LedCount");
                if (cJSON_IsNumber(pPortItem))
                {
                    m_aPorts[i].m_s32LedCount = cJSON_GetNumberValue(pPortItem);
                }
            }
            {
                cJSON *pPortItem = cJSON_GetObjectItemCaseSensitive(pPort, "LedType");
                if (cJSON_IsString(pPortItem))
                {
                    m_aPorts[i].m_sLedType = cJSON_GetStringValue(pPortItem);
                }
            }
        }

        SavePorts();
    }

    pItem = cJSON_GetObjectItemCaseSensitive(json, "LEDPort");
    if (cJSON_IsObject(pItem))
    {
        cJSON * pPortNumber = cJSON_GetObjectItemCaseSensitive(pItem, "PortNumber");
        if (cJSON_IsNumber(pPortNumber))
        {
            int32_t s32PortNumber = cJSON_GetNumberValue(pPortNumber);
            if (s32PortNumber >=0 && s32PortNumber < PROJECT_NUMBER_OF_PORTS)
            {
                {
                    cJSON *pPortItem = cJSON_GetObjectItemCaseSensitive(pItem, "StartUniverse");
                    if (cJSON_IsNumber(pPortItem))
                    {
                        m_aPorts[s32PortNumber].m_s32StartUniverse = cJSON_GetNumberValue(pPortItem);
                    }
                }
                {
                    cJSON *pPortItem = cJSON_GetObjectItemCaseSensitive(pItem, "NoUniverses");
                    if (cJSON_IsNumber(pPortItem))
                    {
                        m_aPorts[s32PortNumber].m_s32NoUniverses = cJSON_GetNumberValue(pPortItem);
                    }
                }
                {
                    cJSON *pPortItem = cJSON_GetObjectItemCaseSensitive(pItem, "LedCount");
                    if (cJSON_IsNumber(pPortItem))
                    {
                        m_aPorts[s32PortNumber].m_s32LedCount = cJSON_GetNumberValue(pPortItem);
                    }
                }
                {
                    cJSON *pPortItem = cJSON_GetObjectItemCaseSensitive(pItem, "LedType");
                    if (cJSON_IsString(pPortItem))
                    {
                        m_aPorts[s32PortNumber].m_sLedType = cJSON_GetStringValue(pPortItem);
                    }
                }

                SavePorts();
            }
        }
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
    cJSON_AddStringToObject(pJson, "Netmask", m_sNetmask.c_str());
    cJSON_AddStringToObject(pJson, "GatewayAddress", m_sGatewayAddress.c_str());
    cJSON_AddStringToObject(pJson, "LedType", m_sLedType.c_str());
    cJSON_AddNumberToObject(pJson, "TimeHigh", m_s32TimeHigh);
    cJSON_AddNumberToObject(pJson, "TimeLow", m_s32TimeLow);
    cJSON_AddNumberToObject(pJson, "StartUniverse", m_s32StartUniverse);
    cJSON_AddNumberToObject(pJson, "NoUniverses", m_s32NoUniverses);
    cJSON_AddStringToObject(pJson, "Identity", m_sIdentity.c_str());
    cJSON_AddStringToObject(pJson, "Model", m_sModel.c_str());
    cJSON_AddStringToObject(pJson, "ProductID", m_sProductID.c_str());
    cJSON_AddBoolToObject(pJson, "ArtNetSync", m_bArtNetSyncEnabled);

    cJSON * pPorts = cJSON_CreateArray();
    for (int32_t i=0; i<PROJECT_NUMBER_OF_PORTS; ++i)
    {
        cJSON * pPort = cJSON_CreateObject();
        cJSON_AddNumberToObject(pPort, "StartUniverse", m_aPorts[i].m_s32StartUniverse);
        cJSON_AddNumberToObject(pPort, "NoUniverses", m_aPorts[i].m_s32NoUniverses);
        cJSON_AddNumberToObject(pPort, "LedCount", m_aPorts[i].m_s32LedCount);
        cJSON_AddStringToObject(pPort, "LedType", m_aPorts[i].m_sLedType.c_str());
        cJSON_AddItemToArray(pPorts, pPort);
    }
    cJSON_AddItemToObject(pJson, "Ports", pPorts);

    return pJson;
}

void Settings::Log()
{
    cJSON * json = ToJson();
    char * pData = cJSON_Print(json);
    ESP_LOGI(TAG, "Settings: \n%s", pData);
    cJSON_Delete(json);
    free(pData);
}

esp_err_t Settings::SetBroadcastSSID(const std::string &sSsid)
{
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "bcast_ssid", sSsid.c_str()));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_sBroadcastSSID = sSsid;
    }
    return err;
}

esp_err_t Settings::SetBroadcastPassword(const std::string &sPassword)
{
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "bcast_pass", sPassword.c_str()));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_sBroadcastPassword = sPassword;
    }
    return err;
}

esp_err_t Settings::SetSiteSSID(const std::string &sSsid)
{
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "site_ssid", sSsid.c_str()));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_sSiteSSID = sSsid;
    }
    return err;
}

esp_err_t Settings::SetSitePassword(const std::string &sPassword)
{
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "site_password", sPassword.c_str()));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_sSitePassword = sPassword;
    }
    return err;
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

esp_err_t Settings::SetNetmask(const std::string &sNetmask)
{
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "netmask", sNetmask.c_str()));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_sNetmask = sNetmask;
    }
    return err;
}

esp_err_t Settings::SetGatewayAddress(const std::string &sGatewayAddress)
{
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "gw_addr", sGatewayAddress.c_str()));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    if (err == ESP_OK)
    {
        m_sGatewayAddress = sGatewayAddress;
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

esp_err_t Settings::SavePorts()
{
    cJSON * json = cJSON_CreateArray();

    for (int32_t i=0; i<PROJECT_NUMBER_OF_PORTS; ++i)
    {
        cJSON * pPort = cJSON_CreateObject();
        cJSON_AddNumberToObject(pPort, "StartUniverse", m_aPorts[i].m_s32StartUniverse);
        cJSON_AddNumberToObject(pPort, "NoUniverses", m_aPorts[i].m_s32NoUniverses);
        cJSON_AddNumberToObject(pPort, "LedCount", m_aPorts[i].m_s32LedCount);
        cJSON_AddStringToObject(pPort, "LedType", m_aPorts[i].m_sLedType.c_str());
        cJSON_AddItemToArray(json, pPort);
    }

    char * pData = cJSON_PrintUnformatted(json);
    ESP_ERROR_CHECK(nvs_set_str(m_s32NVSHandle, "ports", pData));
    esp_err_t err = nvs_commit(m_s32NVSHandle);
    cJSON_Delete(json);
    free(pData);
    return err;
}