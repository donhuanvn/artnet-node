#ifndef __MISCELLANEOUS_H__
#define __MISCELLANEOUS_H__

#include <stdio.h>
#include <memory>
#include <map>

#ifndef PROJECT_DMX_MESSAGE_BUFFER_SIZE
#define PROJECT_DMX_MESSAGE_BUFFER_SIZE 530
#endif

struct cJSON;

class HWStatus
{
public:
    enum Mode
    {
        CONFIG_ONLY,
        CONFIG_AND_RUN_DMX,
    };
    static Mode GetMode();
    static float GetBrightValue();
    static float GetSpeedValue();
};

class DMX512Message
{
    char *pData;

public:
    DMX512Message()
    {
        pData = new char[PROJECT_DMX_MESSAGE_BUFFER_SIZE];
    }
    DMX512Message(char * pBuffer)
    {
        pData = pBuffer;
    }
    ~DMX512Message()
    {
        delete[] pData;
    }
    int32_t GetUniverse();
    char * GetBuffer() { return pData; }
};

namespace Existing
{
    class LedTypeOnline
    {
    public:
        enum TypeLedOnline
        {
            LED1903 = 0,
            LED1904 = 0,
            LED1905 = 0,
            LED2811 = 0,
            LED2812 = 0,
            LED8206 = 0,
            LED8806 = 0,
            LED1916 = 0,
            LED16703 = 0,
            LED9883 = 0,
            LED6803,
            LED1914,
            LED9813,
            LED8903,

            DMX_UCS512A = 5,
            DMX_UCS512D = 5,
            DMX_UCS512H = 5,
            DMX_UCS512CN = 5,
            DMX_UCS512G = 5,

            DMX_UCS512F = 5,
            DMX_UCS512KL = 5,
            DMX_UCS512C1 = 5,
            DMX_UCS512KH = 5,
            DMX_UCS512C4 = 5,
            DMX_UCS512B = 5,
            DMX_UCS512E = 5,

            DMX_SM17512 = 5,
            DMX_SM1752X = 5,
            DMX_SM17500 = 5,
            DMX_SM17500SELF = 5,
            DMX_SM1852X = 5,
            DMX_SM1952X = 5,

            DMX_TM512AB = 5,
            DMX_TM512AL = 5,
            DMX_TM512AC = 5,
            DMX_TM512AD = 5,
            DMX_TM512AE = 5,

            DMX_HI152A0 = 5,
            DMX_HI152A4 = 5,
            DMX_HI152A6 = 5,
            DMX_HI152D = 5,
            DMX_HI152A0SELF = 5,

            DMX_GS8511 = 5,
            DMX_GS8512 = 5,
            DMX_GS8513 = 5,
            DMX_GS8515 = 5,
            DMX_GS8516 = 5,

            DMX_QED512P = 5,

            // DMX_UCS7804= 5,
            DMX_SM1651X3CH = 5,
            DMX_SM1651X4CH = 5,
        };
        static const std::map<std::string, TypeLedOnline> m_mapLedTypes;
        static std::string ToString(TypeLedOnline enumTypeLed);
        static TypeLedOnline FromString(const std::string& sLedType, TypeLedOnline enumDefault = LED1903);
        static bool IsValidLedTypeInt32(int32_t s32LedType);
        static bool IsValidLedTypeString(const std::string& sLedType);
    };


#pragma pack(push) /* push current alignment to stack */
#pragma pack(1)    /* set alignment to 1-byte boundary */

    typedef struct
    {
        uint8_t Id[8];        ///< Array of 8 characters, the final character is a null termination. Value = ‘A’ ‘r’ ‘t’ ‘-‘ ‘N’ ‘e’ ‘t’ 0x00
        uint16_t OpCode;      // Bản tin ArtConfig là 0x2009
        uint8_t ProtVerHi;    ///< High byte of the Art-Net protocol revision number.
        uint8_t ProtVerLo;    ///< Low byte of the Art-Net protocol revision number. Current value 14.
        uint16_t CommandCode; // Mã bản tin.
        uint8_t LengthHi;     ///< The length of the DMX512 data array. This value should be an even number in the range 2 – 512.
        uint8_t Length;       ///< Low Byte of above.
        uint8_t Data[512];    ///< A variable length array of DMX512 lighting data.
    } TArtConfig;

    typedef struct
    {
        uint32_t unique[4];
    } TUniqueID;

    typedef union
    {
        uint32_t IpAddress32;
        uint8_t IpAddress8[4];
    } TIpAddress;

    typedef struct
    {
        TIpAddress IpAddress; // cho người dùng cài đặt
        TIpAddress Netmask;   // mặc định 255.255.255.0
        TIpAddress Gateway;   // mặc định là tiền tố của người dùng cài sau đó chấm 1. ví dụ người dùng cài 192.167.2.3 thì gateway là 192.167.2.1
    } TNetWorkIpInfo;

    typedef struct
    {
        TNetWorkIpInfo NetWorkIpInfo;
        uint8_t MacAddress[6];
        uint8_t aSsid[34];     // Tên mạng wifi
        uint8_t aPassword[34]; // Mật khẩu mạng wifi
    } TNetWorkParams;

#define SHORT_NAME_LENGTH 18
#define LONG_NAME_LENGTH 64
    typedef struct
    {
        uint8_t aShortName[SHORT_NAME_LENGTH]; // Tên thiết bị.
        uint8_t aLongName[LONG_NAME_LENGTH];   // Tên + mô tả thiết bị.
    } TArtNetParams;

    typedef struct
    {
        uint16_t LedType;
        uint16_t NumberUniverses;     /// Số lượng Univer cho mỗi Port
        uint16_t NumberActiveOutputs; /// Số lượng cổng dữ liệu ra
        uint16_t TimeHigh;
        uint16_t TimeLow;
        uint16_t StartUniverse;
    } TPixelRgbParams;

#pragma pack(pop) /* restore original alignment from stack */

    typedef struct
    {
        TUniqueID UniqueID; // Cái này là mã Unique của từng mạch, có sẵn trong phần cứng chip, ko bao giờ giống nhau.
        TNetWorkParams NetWorkParams;
        TArtNetParams ArtNetParams;
        TPixelRgbParams PixelRgbParams;
    } TProjectParams;

    typedef enum
    {
        ConfigGetConfig = 0x1908,
        ConfigSetConfig = 0x1107,
        ConfigErrorParam = 0x1995,
        ConfigError = 0x2021,
        ConfigIgnore = 0x2022,
    } T_ConfigDmxCommand;

    typedef struct NQN_MANAGER_VERSION
    {
        uint8_t MainVersionHigh;
        uint8_t MainVersionLow;
        uint8_t SubVersionHigh;
        uint8_t SubVersionLow;
        uint8_t Year;
        uint8_t Month;
        uint8_t Date;
        uint8_t Hour;
        uint8_t Minute;
        uint8_t Second;

        NQN_MANAGER_VERSION() = default;
        NQN_MANAGER_VERSION(const std::string &sVersion, const std::string &sBuildTime);
    } NQN_MANAGER_VERSION;

    typedef struct
    {
        uint8_t SubProductLow;
        uint8_t SubProductHight;
        uint8_t MainProductLow;
        uint8_t MainProductHight;
    } NQN_MANAGER_PRODUCT;

    void GetConfig(TArtConfig &stArtConfig, size_t& u32Size);
    void SetConfig(const TArtConfig &stArtConfig, TArtConfig &stArtConfigReturn, size_t& u32Size);
}

#endif /* __MISCELLANEOUS_H__ */