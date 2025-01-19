#ifndef __ARTNET_NODE_PORT_H__
#define __ARTNET_NODE_PORT_H__

#include "config.h"
#include <mutex>
#include <memory>
#include <array>
#include <list>
#include "models/settings.h"
#include "miscellaneous.h"
#include "FastLED.h"

class Port
{
public:
    const int32_t m_s32PortNumber;

    std::mutex m_oBufferMutex;
    std::array<CRGB, PROJECT_MAXIMUM_NUMBER_OF_LEDS_PER_PORT> * m_pBuffer;
    std::list<std::shared_ptr<DMX512Message>> m_listMessages;
    int32_t m_s32StartUniv;
    int32_t m_s32EndUniv;
    int32_t m_s32NextUniv;
    CLEDController *m_oCLedController;

    esp_err_t Init();
    inline bool MatchUniv(int32_t s32Univ);
    inline bool MatchUnivLoss(int32_t s32Univ);

public:
    Port(int32_t s32PortNumber);
    inline bool IsFull();
    void Commit();
    void TryAddMessage(std::shared_ptr<DMX512Message> spMessage);
};

class Ports
{
    std::array<Port *, PROJECT_NUMBER_OF_PORTS> m_aPortList;
    bool m_bSync;

public:
    static Ports &GetInstance()
    {
        static Ports oIns;
        return oIns;
    }
    void Init();
    static void FreeRTOSTask(void * pvParameters);
    void Sync() { m_bSync = true; }
    void HandleDMXMessage(std::shared_ptr<DMX512Message> spMsg);
};

#endif /* __ARTNET_NODE_PORT_H__ */
