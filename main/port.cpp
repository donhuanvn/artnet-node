#include "port.h"
#include <string.h>
#include "esp_check.h"
#include "esp_log.h"
#include "miscellaneous.h"

static const char *TAG = "Port";

static std::array<std::array<CRGB, PROJECT_MAXIMUM_NUMBER_OF_LEDS_PER_PORT>, PROJECT_NUMBER_OF_PORTS> g_buffers; // 0-based index

static CLEDController *CLEDControllerFactory(const std::string &sChipset, int32_t s32Port, CRGB *pData, int32_t s32LedCount)
{
    if (sChipset == "LED1903" || sChipset == "LED16703")
    {
        switch (s32Port)
        {
        case 0:
            return &FastLED.addLeds<SM16703, PROJECT_PORT_0_DATA_PIN, RBG>(pData, s32LedCount);
        case 1:
            return &FastLED.addLeds<SM16703, PROJECT_PORT_1_DATA_PIN, RBG>(pData, s32LedCount);
        case 2:
            return &FastLED.addLeds<SM16703, PROJECT_PORT_2_DATA_PIN, RBG>(pData, s32LedCount);
        case 3:
            return &FastLED.addLeds<SM16703, PROJECT_PORT_3_DATA_PIN, RBG>(pData, s32LedCount);
        case 4:
            return &FastLED.addLeds<SM16703, PROJECT_PORT_4_DATA_PIN, RBG>(pData, s32LedCount);
        case 5:
            return &FastLED.addLeds<SM16703, PROJECT_PORT_5_DATA_PIN, RBG>(pData, s32LedCount);
        case 6:
            return &FastLED.addLeds<SM16703, PROJECT_PORT_6_DATA_PIN, RBG>(pData, s32LedCount);
        case 7:
            return &FastLED.addLeds<SM16703, PROJECT_PORT_7_DATA_PIN, RBG>(pData, s32LedCount);
        default:
            ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
        }
    }
    else if (sChipset == "UCS1903")
    {
        switch (s32Port)
        {
        case 0:
            return &FastLED.addLeds<UCS1903B, PROJECT_PORT_0_DATA_PIN>(pData, s32LedCount);
        case 1:
            return &FastLED.addLeds<UCS1903B, PROJECT_PORT_1_DATA_PIN>(pData, s32LedCount);
        case 2:
            return &FastLED.addLeds<UCS1903B, PROJECT_PORT_2_DATA_PIN>(pData, s32LedCount);
        case 3:
            return &FastLED.addLeds<UCS1903B, PROJECT_PORT_3_DATA_PIN>(pData, s32LedCount);
        case 4:
            return &FastLED.addLeds<UCS1903B, PROJECT_PORT_4_DATA_PIN>(pData, s32LedCount);
        case 5:
            return &FastLED.addLeds<UCS1903B, PROJECT_PORT_5_DATA_PIN>(pData, s32LedCount);
        case 6:
            return &FastLED.addLeds<UCS1903B, PROJECT_PORT_6_DATA_PIN>(pData, s32LedCount);
        case 7:
            return &FastLED.addLeds<UCS1903B, PROJECT_PORT_7_DATA_PIN>(pData, s32LedCount);
        default:
            ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
        }
    }
    else
    {
        ESP_ERROR_CHECK(ESP_ERR_NOT_SUPPORTED);
    }
    // Unreachable return!
    return NULL;
}

static bool CheckPortNumber(int32_t s32Port)
{
    return 0 <= s32Port && s32Port < PROJECT_NUMBER_OF_PORTS;
}

static bool CheckLedCount(int32_t s32LedCount)
{
    return 0 <= s32LedCount && s32LedCount <= PROJECT_MAXIMUM_NUMBER_OF_LEDS_PER_PORT; 
}

static bool CheckLedType(const std::string& sLedType)
{
    return Existing::LedTypeOnline::IsValidLedTypeString(sLedType);
}

Port::Port(int32_t s32PortNumber) : m_s32PortNumber(s32PortNumber)
{
    ESP_ERROR_CHECK(Init());
    ESP_LOGI(TAG, "Init port %ld successfully!", m_s32PortNumber);
}

esp_err_t Port::Init()
{
    ESP_RETURN_ON_FALSE(CheckPortNumber(m_s32PortNumber), ESP_ERR_NOT_SUPPORTED, TAG, "Invalid Port Number %ld", m_s32PortNumber);
    m_pBuffer = &g_buffers[m_s32PortNumber];

    m_s32StartUniv = Settings::GetInstance().GetStartUniverse(m_s32PortNumber);
    m_s32EndUniv = m_s32StartUniv + Settings::GetInstance().GetNoUniverses(m_s32PortNumber);
    m_s32NextUniv = m_s32StartUniv;

    std::string sLedType = Settings::GetInstance().GetLedType(m_s32PortNumber);
    int32_t s32LedCount = Settings::GetInstance().GetLedCount(m_s32PortNumber);
    ESP_LOGI(TAG, "Initializing Port %ld, Led Type %s, Led Count %ld", m_s32PortNumber, sLedType.c_str(), s32LedCount);
    ESP_RETURN_ON_FALSE(CheckLedType(sLedType), ESP_ERR_NOT_SUPPORTED, TAG, "Port %ld: Invalid Led Type %s", m_s32PortNumber, sLedType.c_str());
    ESP_RETURN_ON_FALSE(CheckLedCount(s32LedCount), ESP_ERR_NOT_SUPPORTED, TAG, "Port %ld: Invalid Led Count %ld", m_s32PortNumber, s32LedCount);

    m_oCLedController = CLEDControllerFactory(sLedType, m_s32PortNumber, m_pBuffer->data(), s32LedCount);

    return ESP_OK;
}

bool Port::MatchUniv(int32_t s32Univ)
{
    return m_s32EndUniv != s32Univ && m_s32NextUniv == s32Univ;
}

bool Port::MatchUnivLoss(int32_t s32Univ)
{
    return !MatchUniv(s32Univ) && m_s32StartUniv == s32Univ;
}

bool Port::IsFull()
{
    return m_s32EndUniv == m_s32NextUniv;
}

void Port::Commit()
{
    // ESP_LOGI(TAG, "Commit on port %ld", m_s32PortNumber);
    if (m_oBufferMutex.try_lock())
    {
        void * pDestBuffer = m_pBuffer->data();
        int32_t s32Offset = 0;
        for(auto spMsg : m_listMessages)
        {
            void * pSrcBuffer = spMsg->GetBuffer() + 18;
            size_t s32Size = ((spMsg->GetBuffer()[16] << 8) | spMsg->GetBuffer()[17]) - 2; // 510 bytes, 170 leds
            memcpy(pDestBuffer + s32Offset, pSrcBuffer, s32Size);
            s32Offset += s32Size;
        }
        // ESP_LOGI(TAG, "Bytes of led 170 %d, %d, %d", m_pBuffer->at(169).raw[0], m_pBuffer->at(169).raw[1], m_pBuffer->at(169).raw[2]);
        // ESP_LOGI(TAG, "Bytes of led 170 %d, %d, %d", *(m_listMessages.front()->GetBuffer() + 18 + 507), *(m_listMessages.front()->GetBuffer() + 18 + 508), *(m_listMessages.front()->GetBuffer() + 18 + 509));
        m_oBufferMutex.unlock();
        m_s32NextUniv = m_s32StartUniv;
        m_listMessages.clear();
    }
}

void Port::TryAddMessage(std::shared_ptr<DMX512Message> spMessage)
{
    if (!IsFull())
    {
        if (MatchUniv(spMessage->GetUniverse()))
        {
            // ESP_LOGI(TAG, "Received Matched Universe (%ld) Message for Port %ld", spMessage->GetUniverse(),  m_s32PortNumber);
            m_listMessages.push_back(spMessage);
            m_s32NextUniv++;
        }
        else if (MatchUnivLoss(spMessage->GetUniverse()))
        {
            // ESP_LOGI(TAG, "Workaround for message loss on Port %ld", m_s32PortNumber);
            m_listMessages.clear();
            m_listMessages.push_back(spMessage);
            m_s32NextUniv = m_s32StartUniv + 1;
        }
    }
}

void Ports::Init()
{
    for (int32_t i = 0; i < PROJECT_NUMBER_OF_PORTS; ++i)
    {
        m_aPortList[i] = new Port(i);
    }
}

void Ports::HandleDMXMessage(std::shared_ptr<DMX512Message> spMsg)
{
    for (int32_t i = 0; i < PROJECT_NUMBER_OF_PORTS; ++i)
    {
        m_aPortList[i]->TryAddMessage(spMsg);
        if (m_aPortList[i]->IsFull())
        {
            m_aPortList[i]->Commit();
        }
    }
}

void Ports::FreeRTOSTask(void * pvParameters)
{
    while(true)
    {
        if (Ports::GetInstance().m_bSync)
        {
            Ports::GetInstance().m_bSync = false;
            for (int32_t i = 0; i < PROJECT_NUMBER_OF_PORTS; ++i)
            {
                Ports::GetInstance().m_aPortList[i]->m_oBufferMutex.lock();
            }
            FastLED.show();
            for (int32_t i = 0; i < PROJECT_NUMBER_OF_PORTS; ++i)
            {
                Ports::GetInstance().m_aPortList[i]->m_oBufferMutex.unlock();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}