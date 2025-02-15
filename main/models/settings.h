#ifndef __ARTNET_NODE_SETTINGS_MODEL_H__
#define __ARTNET_NODE_SETTINGS_MODEL_H__

#include <stdio.h>
#include <string>
#include <esp_err.h>
#include "cJSON.h"
#include "nvs.h"
#include <list>
#include <array>

#ifndef PROJECT_NUMBER_OF_PORTS
#define PROJECT_NUMBER_OF_PORTS 4
#endif

#ifndef PROJECT_MAXIMUM_NUMBER_OF_UNIVERSES
#define PROJECT_MAXIMUM_NUMBER_OF_UNIVERSES 40
#endif



class Settings
{
    typedef struct PortSettings
    {
        int32_t m_s32StartUniverse;
        int32_t m_s32NoUniverses;
        int32_t m_s32LedCount;
        std::string m_sLedType;
        PortSettings() : m_s32StartUniverse(-1), m_s32NoUniverses(-1), m_s32LedCount(-1) {}
    } PortSettings;

    std::string m_sBroadcastSSID;
    std::string m_sBroadcastPassword;
    std::string m_sSiteSSID;
    std::string m_sSitePassword;
    std::string m_sStaticIP;
    std::string m_sLedType;
    int32_t m_s32TimeHigh;
    int32_t m_s32TimeLow;
    int32_t m_s32StartUniverse;
    int32_t m_s32NoUniverses;
    std::string m_sIdentity;
    std::string m_sModel;
    std::string m_sProductID;
    bool m_bArtNetSyncEnabled;
    std::array<PortSettings, PROJECT_NUMBER_OF_PORTS> m_aPorts; // 0-based index

    nvs_handle_t m_s32NVSHandle;

    esp_err_t SavePorts();

public:
    static Settings &GetInstance()
    {
        static Settings oIns;
        return oIns;
    }
    Settings();

    void FromJson(const cJSON *json);
    cJSON *ToJson();
    void Log();

    const std::string &GetBroadcastSSID() const { return m_sBroadcastSSID; }
    esp_err_t SetBroadcastSSID(const std::string &sSsid);

    const std::string &GetBroadcastPassword() const { return m_sBroadcastPassword; }
    esp_err_t SetBroadcastPassword(const std::string &sPassword);

    const std::string &GetSiteSSID() const { return m_sSiteSSID; }
    esp_err_t SetSiteSSID(const std::string &sSsid);

    const std::string &GetSitePassword() const { return m_sSitePassword; }
    esp_err_t SetSitePassword(const std::string &sPassword);

    const std::string &GetStaticIP() const { return m_sStaticIP; }
    esp_err_t SetStaticIP(const std::string &sIP);

    const std::string &GetLedType() const { return m_sLedType; }
    esp_err_t SetLedType(const std::string &sType);

    int32_t GetTimeHigh() const { return m_s32TimeHigh; }
    esp_err_t SetTimeHigh(int32_t s32Time);

    int32_t GetTimeLow() const { return m_s32TimeLow; }
    esp_err_t SetTimeLow(int32_t s32Time);

    int32_t GetStartUniverse() const { return m_s32StartUniverse; }
    esp_err_t SetStartUniverse(int32_t s32Univ);

    int32_t GetNoUniverses() const { return m_s32NoUniverses; }
    esp_err_t SetNoUniverses(int32_t s32Count);

    const std::string &GetIdentity() const { return m_sIdentity; }
    esp_err_t SetIdentity(const std::string &sIdenityNew);

    const std::string &GetModel() const { return m_sModel; }
    esp_err_t SetModel(const std::string &sModelNew);

    const std::string &GetProductID() const { return m_sProductID; }
    esp_err_t SetProductID(const std::string &sProductIDNew);

    bool GetArtNetSyncEnabled() const { return m_bArtNetSyncEnabled; }
    esp_err_t SetArtNetSyncEnabled(bool bEnabled);

    int32_t GetStartUniverse(int32_t s32PortNumber) const { return m_aPorts[s32PortNumber].m_s32StartUniverse; }
    int32_t GetNoUniverses(int32_t s32PortNumber) const { return m_aPorts[s32PortNumber].m_s32NoUniverses; }
    int32_t GetLedCount(int32_t s32PortNumber) const { return m_aPorts[s32PortNumber].m_s32LedCount; }
    std::string GetLedType(int32_t s32PortNumber) const { return m_aPorts[s32PortNumber].m_sLedType; }
};

#endif /* __ARTNET_NODE_SETTINGS_MODEL_H__ */