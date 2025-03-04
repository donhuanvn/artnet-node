#include "miscellaneous.h"
#include "models/settings.h"
#include "models/info.h"
#include "lwip/inet.h"
#include "driver/gpio.h"
#include "config.h"
#include <string.h>
#include "esp_mac.h"
#include "esp_log.h"
#include "version.h"

static const char * TAG = "Misc";

HWStatus::Mode HWStatus::GetMode()
{
    int32_t s32Level = gpio_get_level(static_cast<gpio_num_t>(PROJECT_GPIO_INPUT_MODE_SELECT_0));
    switch (s32Level)
    {
    case 0:
        return WIFI_AP_ONLY;
    default:
        return WIFI_AUTO_CONNECT;
    }
}

float HWStatus::GetBrightValue()
{
    // Todo
    return -1.0;
}

float HWStatus::GetSpeedValue()
{
    // Todo
    return -1.0;
}

int32_t DMX512Message::GetUniverse()
{
    return pData[15] << 8 | pData[14];
}

namespace Existing
{
    using TypeLedOnline = LedTypeOnline::TypeLedOnline;

    const std::map<std::string, TypeLedOnline> LedTypeOnline::m_mapLedTypes = 
    {
        {"LED1903", LedTypeOnline::LED1903},
        {"LED1904", LedTypeOnline::LED1904},
        {"LED1905", LedTypeOnline::LED1905},
        {"LED2811", LedTypeOnline::LED2811},
        {"LED2812", LedTypeOnline::LED2812},
        {"LED8206", LedTypeOnline::LED8206},
        {"LED8806", LedTypeOnline::LED8806},
        {"LED1916", LedTypeOnline::LED1916},
        {"LED16703", LedTypeOnline::LED16703},
        {"LED9883", LedTypeOnline::LED9883},
        {"LED6803", LedTypeOnline::LED6803},
        {"LED1914", LedTypeOnline::LED1914},
        {"LED9813", LedTypeOnline::LED9813},
        {"LED8903", LedTypeOnline::LED8903},
        {"DMX_UCS512A", LedTypeOnline::DMX_UCS512A},
        {"DMX_UCS512D", LedTypeOnline::DMX_UCS512D},
        {"DMX_UCS512H", LedTypeOnline::DMX_UCS512H},
        {"DMX_UCS512CN", LedTypeOnline::DMX_UCS512CN},
        {"DMX_UCS512G", LedTypeOnline::DMX_UCS512G},
        {"DMX_UCS512F", LedTypeOnline::DMX_UCS512F},
        {"DMX_UCS512KL", LedTypeOnline::DMX_UCS512KL},
        {"DMX_UCS512C1", LedTypeOnline::DMX_UCS512C1},
        {"DMX_UCS512KH", LedTypeOnline::DMX_UCS512KH},
        {"DMX_UCS512C4", LedTypeOnline::DMX_UCS512C4},
        {"DMX_UCS512B", LedTypeOnline::DMX_UCS512B},
        {"DMX_UCS512E", LedTypeOnline::DMX_UCS512E},
        {"DMX_SM17512", LedTypeOnline::DMX_SM17512},
        {"DMX_SM1752X", LedTypeOnline::DMX_SM1752X},
        {"DMX_SM17500", LedTypeOnline::DMX_SM17500},
        {"DMX_SM17500SELF", LedTypeOnline::DMX_SM17500SELF},
        {"DMX_SM1852X", LedTypeOnline::DMX_SM1852X},
        {"DMX_SM1952X", LedTypeOnline::DMX_SM1952X},
        {"DMX_TM512AB", LedTypeOnline::DMX_TM512AB},
        {"DMX_TM512AL", LedTypeOnline::DMX_TM512AL},
        {"DMX_TM512AC", LedTypeOnline::DMX_TM512AC},
        {"DMX_TM512AD", LedTypeOnline::DMX_TM512AD},
        {"DMX_TM512AE", LedTypeOnline::DMX_TM512AE},
        {"DMX_HI152A0", LedTypeOnline::DMX_HI152A0},
        {"DMX_HI152A4", LedTypeOnline::DMX_HI152A4},
        {"DMX_HI152A6", LedTypeOnline::DMX_HI152A6},
        {"DMX_HI152D", LedTypeOnline::DMX_HI152D},
        {"DMX_HI152A0SELF", LedTypeOnline::DMX_HI152A0SELF},
        {"DMX_GS8511", LedTypeOnline::DMX_GS8511},
        {"DMX_GS8512", LedTypeOnline::DMX_GS8512},
        {"DMX_GS8513", LedTypeOnline::DMX_GS8513},
        {"DMX_GS8515", LedTypeOnline::DMX_GS8515},
        {"DMX_GS8516", LedTypeOnline::DMX_GS8516},
        {"DMX_QED512P", LedTypeOnline::DMX_QED512P},
        // {"DMX_UCS7804", LedTypeOnline::DMX_UCS7804},
        {"DMX_SM1651X3CH", LedTypeOnline::DMX_SM1651X3CH},
        {"DMX_SM1651X4CH", LedTypeOnline::DMX_SM1651X4CH}
    };

    std::string LedTypeOnline::ToString(TypeLedOnline enumTypeLed)
    {
        for (const auto& pair : m_mapLedTypes)
        {
            if (pair.second == enumTypeLed)
            {
                return pair.first;
            }
        }
        return "Unknown";
    }

    TypeLedOnline LedTypeOnline::FromString(const std::string& sLedType, TypeLedOnline enumDefault)
    {
        auto it = m_mapLedTypes.find(sLedType);
        if (it != m_mapLedTypes.end())
        {
            return it->second;
        }
        return enumDefault;
    }

    bool LedTypeOnline::IsValidLedTypeInt32(int32_t s32LedType)
    {
        for (const auto& pair : m_mapLedTypes)
        {
            if (static_cast<int32_t>(pair.second) == s32LedType)
            {
                return true;
            }
        }
        return false;
    }

    bool LedTypeOnline::IsValidLedTypeString(const std::string& sLedType)
    {
        return m_mapLedTypes.find(sLedType) != m_mapLedTypes.end();
    }

    NQN_MANAGER_VERSION::NQN_MANAGER_VERSION(const std::string &sVersion, const std::string &sBuildTime)
    {
        // Parse version string
        // Example sVersion: "1.0.0.0"
        sscanf(sVersion.c_str(), "%hhu.%hhu.%hhu.%hhu", &MainVersionHigh, &MainVersionLow, &SubVersionHigh, &SubVersionLow);
    
        // Parse build time string
        // Example sBuildTime: "2025-02-23 18:01:25"
        int32_t s32Year;
        sscanf(sBuildTime.c_str(), "%ld-%hhu-%hhu %hhu:%hhu:%hhu", &s32Year, &Month, &Date, &Hour, &Minute, &Second);
    
        Year = static_cast<uint8_t>(s32Year - 2000); // Assuming year is in 2000s
    }

    static void BuildConfig(TProjectParams *pstParams)
    {
        memset(&pstParams->UniqueID, 0, sizeof(pstParams->UniqueID));

        std::string sIp = InfoModel::GetInstance().GetIP();
        std::string sNetmask = InfoModel::GetInstance().GetNetmask();
        std::string sGwAddr = InfoModel::GetInstance().GetGatewayAddress();
        pstParams->NetWorkParams.NetWorkIpInfo.IpAddress.IpAddress32 = inet_addr(sIp.c_str());
        pstParams->NetWorkParams.NetWorkIpInfo.Netmask.IpAddress32 = inet_addr(sNetmask.c_str());
        pstParams->NetWorkParams.NetWorkIpInfo.Gateway.IpAddress32 = inet_addr(sGwAddr.c_str());
        esp_read_mac(pstParams->NetWorkParams.MacAddress, ESP_MAC_WIFI_STA);
        strncpy((char *)pstParams->NetWorkParams.aSsid, Settings::GetInstance().GetSiteSSID().c_str(), sizeof(pstParams->NetWorkParams.aSsid));
        strncpy((char *)pstParams->NetWorkParams.aPassword, Settings::GetInstance().GetSitePassword().c_str(), sizeof(pstParams->NetWorkParams.aPassword));
        strncpy((char *)pstParams->ArtNetParams.aShortName, Settings::GetInstance().GetIdentity().c_str(), sizeof(pstParams->ArtNetParams.aShortName));
        strncpy((char *)pstParams->ArtNetParams.aLongName, Settings::GetInstance().GetModel().c_str(), sizeof(pstParams->ArtNetParams.aLongName));
        pstParams->PixelRgbParams.LedType = LedTypeOnline::FromString(Settings::GetInstance().GetLedType());
        pstParams->PixelRgbParams.NumberUniverses = Settings::GetInstance().GetNoUniverses();
        pstParams->PixelRgbParams.StartUniverse = Settings::GetInstance().GetStartUniverse();
        pstParams->PixelRgbParams.NumberActiveOutputs = PROJECT_NUMBER_OF_PORTS;
        pstParams->PixelRgbParams.TimeHigh = Settings::GetInstance().GetTimeHigh();
        pstParams->PixelRgbParams.TimeLow = Settings::GetInstance().GetTimeLow();
    }

    static void BuildVersion(NQN_MANAGER_VERSION *pstVersion)
    {
        *pstVersion = NQN_MANAGER_VERSION(FWVersion::version, FWVersion::buildTime);
    }

    static void BuildProductID(NQN_MANAGER_PRODUCT *pstProductId)
    {
    }

    static uint8_t NqnProject_CheckValidParams(TProjectParams *_ProjectParams)
    {
        uint8_t IsValid = 1;
        IsValid &= ip4_addr_netmask_valid(_ProjectParams->NetWorkParams.NetWorkIpInfo.Netmask.IpAddress32);
        IsValid &= LedTypeOnline::IsValidLedTypeInt32(_ProjectParams->PixelRgbParams.LedType);
        IsValid &= (_ProjectParams->PixelRgbParams.NumberActiveOutputs >= 1) && (_ProjectParams->PixelRgbParams.NumberActiveOutputs <= PROJECT_NUMBER_OF_PORTS);
        IsValid &= (_ProjectParams->PixelRgbParams.StartUniverse <= 32768);
        IsValid &= (_ProjectParams->PixelRgbParams.NumberUniverses >= 1);
        // IsValid &= (_ProjectParams->PixelRgbParams.NumberUniverses <= MAX_PAGE_SIZE);
        IsValid &= (SettingsValidator::IsValidTimeHigh(_ProjectParams->PixelRgbParams.TimeHigh));
        IsValid &= (SettingsValidator::IsValidTimeLow(_ProjectParams->PixelRgbParams.TimeLow));
        // IsValid &= (_ProjectParams->PixelRgbParams.NumberUniverses * _ProjectParams->PixelRgbParams.NumberActiveOutputs) <= ARTNET_MAX_NUMBER_UNIVERSER;
        return IsValid;
    }

    static void NqnProject_SaveParams(TProjectParams *pstParams)
    {
        Settings::GetInstance().SetStaticIP(inet_ntoa(pstParams->NetWorkParams.NetWorkIpInfo.IpAddress));
        Settings::GetInstance().SetNetmask(inet_ntoa(pstParams->NetWorkParams.NetWorkIpInfo.Netmask));
        Settings::GetInstance().SetGatewayAddress(inet_ntoa(pstParams->NetWorkParams.NetWorkIpInfo.Gateway));
        Settings::GetInstance().SetSiteSSID((char *)pstParams->NetWorkParams.aSsid);
        Settings::GetInstance().SetSitePassword((char *)pstParams->NetWorkParams.aPassword);
        Settings::GetInstance().SetIdentity((char *)pstParams->ArtNetParams.aShortName);
        Settings::GetInstance().SetModel((char *)pstParams->ArtNetParams.aLongName);
        Settings::GetInstance().SetLedType(LedTypeOnline::ToString((LedTypeOnline::TypeLedOnline)pstParams->PixelRgbParams.LedType));
        Settings::GetInstance().SetNoUniverses(pstParams->PixelRgbParams.NumberUniverses);
        Settings::GetInstance().SetStartUniverse(pstParams->PixelRgbParams.StartUniverse);
        Settings::GetInstance().SetTimeHigh(pstParams->PixelRgbParams.TimeHigh);
        Settings::GetInstance().SetTimeLow(pstParams->PixelRgbParams.TimeLow);
    }

    static uint8_t HardWare_CompareUniqueID(TUniqueID *out)
    {
        if ((out->unique[0] == 0xFFFFFFFF) && (out->unique[1] == 0xFFFFFFFF) && (out->unique[2] == 0xFFFFFFFF) && (out->unique[3] == 0xFFFFFFFF))
            return 1;
        // if (out->unique[0] != *(volatile uint32_t *)(0x1FFFF7E8))
        //     return 0;
        // if (out->unique[1] != *(volatile uint32_t *)(0x1FFFF7EC))
        //     return 0;
        // if (out->unique[2] != *(volatile uint32_t *)(0x1FFFF7F0))
        //     return 0;
        // if (out->unique[3] != out->unique[0] + out->unique[1] + out->unique[2])
        //     return 0;
        return 1;
    }

    void GetConfig(TArtConfig &stArtConfig, size_t& u32Size)
    {
        static const size_t u32LengthAll = sizeof(NQN_MANAGER_PRODUCT) + sizeof(NQN_MANAGER_VERSION) + sizeof(TProjectParams);

        memcpy(stArtConfig.Id, "Art-Net", sizeof(stArtConfig.Id));
        stArtConfig.OpCode = 0x2009;
        stArtConfig.ProtVerHi = 1;
        stArtConfig.ProtVerLo = 57;
        stArtConfig.CommandCode = T_ConfigDmxCommand::ConfigGetConfig;
        stArtConfig.LengthHi = u32LengthAll >> 8;
        stArtConfig.Length = u32LengthAll & 0xFF;

        BuildConfig((TProjectParams *)&stArtConfig.Data[0]);
        BuildVersion((NQN_MANAGER_VERSION *)&stArtConfig.Data[sizeof(TProjectParams)]);
        BuildProductID((NQN_MANAGER_PRODUCT *)&stArtConfig.Data[sizeof(TProjectParams) + sizeof(NQN_MANAGER_VERSION)]);

        uint8_t *p = stArtConfig.Data;
        uint8_t u8CheckSum = 0;
        for (uint32_t i = 0; i < u32LengthAll; i++)
        {
            u8CheckSum += *p++;
        }
        stArtConfig.Data[u32LengthAll] = u8CheckSum + 2;

        u32Size = 16 + u32LengthAll + 1;
    }

    void SetConfig(const TArtConfig &stArtConfig, TArtConfig &stArtConfigReturn, size_t& u32Size)
    {
        if (stArtConfig.CommandCode == ConfigSetConfig && HardWare_CompareUniqueID((TUniqueID *)stArtConfig.Data))
        {
            uint8_t *p = (uint8_t *)stArtConfig.Data;
            uint8_t CheckSum = 0;
            for (size_t i = 0; i < sizeof(TProjectParams); i++)
            {
                CheckSum += *p++;
            }
            if ((CheckSum + 2) == stArtConfig.Data[sizeof(TProjectParams)])
            {
                TProjectParams *Params = (TProjectParams *)stArtConfig.Data;
                if (NqnProject_CheckValidParams(Params))
                {
                    ESP_LOGI(TAG, "Received valid config");
                    NqnProject_SaveParams(Params);
                    GetConfig(stArtConfigReturn, u32Size);
                    stArtConfigReturn.CommandCode = ConfigSetConfig;
                }
                else
                {
                    ESP_LOGI(TAG, "Received config is invalid!");
                    GetConfig(stArtConfigReturn, u32Size);
                    stArtConfigReturn.CommandCode = ConfigErrorParam;
                }
            }
        }
    }
}
