#include "status.h"
#include "esp_log.h"
#include "settings.h"

#ifndef PROJECT_WINDOW_MESSAGE_COUNT
#define PROJECT_WINDOW_MESSAGE_COUNT 1000
#endif

static const char * TAG = "Status-Model";

Status::Status()
{
    m_s64DMXCount = 0;
    m_f32ReceptionRate = 0;
    m_s32LastUniv = -1;
    m_s32WindowLostCount = 0;
    m_s32WindowCount = 0;
}

void Status::UpdateForNewDMXMessage(int32_t s32Univ)
{
    m_s64DMXCount++;

    s32Univ -= Settings::GetInstance().GetStartUniverse(); // offset to 0.
    if (m_s32LastUniv != -1)
    {
        if (m_s32LastUniv < s32Univ)
        {
            m_s32WindowLostCount += (s32Univ - m_s32LastUniv - 1);
        }
        else
        {
            m_s32WindowLostCount += (Settings::GetInstance().GetNoUniverses() - 1 + s32Univ - m_s32LastUniv);
        }
    }
    m_s32LastUniv = s32Univ;

    m_s32WindowCount++;
    if (m_s32WindowCount >= PROJECT_WINDOW_MESSAGE_COUNT)
    {
        m_f32ReceptionRate = (float)m_s32WindowCount / (m_s32WindowCount + m_s32WindowLostCount);
        m_s32WindowCount = 0;
        m_s32WindowLostCount = 0;
    }
}

cJSON * Status::ToJson()
{
    cJSON * json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "DMXCount", m_s64DMXCount);
    cJSON_AddNumberToObject(json, "ReceptionRate", m_f32ReceptionRate);
    return json;
}

void Status::Log()
{
    cJSON * json = ToJson();
    char * pData = cJSON_Print(json);
    ESP_LOGI(TAG, "Status:\n%s", pData);
    cJSON_Delete(json);
    free(pData);
}