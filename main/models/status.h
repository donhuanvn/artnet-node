#ifndef __ARTNET_NODE_STATUS_MODEL_H__
#define __ARTNET_NODE_STATUS_MODEL_H__

#include <stdio.h>
#include <string>
#include <esp_err.h>
#include "cJSON.h"

class Status
{
    int64_t m_s64DMXCount;
    float m_f32ReceptionRate;

    int32_t m_s32LastUniv;
    int32_t m_s32WindowLostCount;
    int32_t m_s32WindowCount;
public:
    static Status& GetInstance()
    {
        static Status oIns;
        return oIns;
    }
    Status();
    void UpdateForNewDMXMessage(int32_t s32Univ);
    cJSON * ToJson();
    void Log();
};

#endif /* __ARTNET_NODE_STATUS_MODEL_H__ */