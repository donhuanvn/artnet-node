#include "port.h"
#include "functional"
#include "driver/gptimer.h"

class Timer
{
    gptimer_handle_t m_Handle;
    gptimer_config_t m_Config;
public:
    static Timer& GetInstance()
    {
        static Timer oIns;
        return oIns;
    }
    Timer()
    {
        m_Config = {
            .clk_src = GPTIMER_CLK_SRC_DEFAULT,
            .direction = GPTIMER_COUNT_UP,
            .resolution_hz = 4000000, // 4MHz, 1 tick=0.25us
        };
        ESP_ERROR_CHECK(gptimer_new_timer(&m_Config, &m_Handle));
        
    }
    void RegisterCallback(std::function<void(void)> oCb)
    {
        
    }
};


class Batch : public std::vector<DMX512MessageShared>
{
    const Port * m_pPort;
    std::array<bool, PROJECT_MAXIMUM_NUMBER_OF_UNIVERSES> m_aReceived;
public:
    Batch(const Port * pPort) : m_pPort(pPort)
    {
        m_aReceived.fill(false);
        std::vector<DMX512MessageShared>::reserve(Settings::GetInstance().QueryUnivCount(pPort->m_s32PortNumber));
    }
    void push_back(const DMX512MessageShared &spMsg)
    {
        int32_t s32Univ = spMsg->GetUniverse();
        if (Settings::GetInstance().IsBelongToPort(s32Univ, m_pPort->m_s32PortNumber) == true)
        {
            if (m_aReceived[s32Univ] == true)
            {
                // Reset the batch (workaround for message loss)
                std::vector<DMX512MessageShared>::clear();
                m_aReceived.fill(false);
            }
            std::vector<DMX512MessageShared>::push_back(spMsg);
            m_aReceived[s32Univ] = true;
        }
    }
    bool enough()
    {
        return Settings::GetInstance().QueryUnivCount(m_pPort->m_s32PortNumber) == std::vector<DMX512MessageShared>::size();
    }
};

class Sender
{
public:
    virtual ~Sender() = 0;
};

class DMXSender : public Sender
{
    ~DMXSender()
    {

    }
    bool IsIdeal()
    {
        return false;
    }
};

class UCS1903Sender : public Sender
{
    ~UCS1903Sender()
    {

    }
};

Port::Port(int32_t s32Number, Port::ProtocolType eProtocolType) : m_s32PortNumber(s32Number)
{
    m_eType = eProtocolType;
    if (eProtocolType == ProtocolType::DMX)
    {
        m_pSender = new DMXSender();
    }
    else if (eProtocolType == ProtocolType::UCS1903)
    {
        m_pSender = new UCS1903Sender();
    }
    else
    {
        ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
    }

    m_spIncommingBatch = std::make_shared<Batch>(this);
    m_spOutcommingBatch = std::make_shared<Batch>(this);
}

Port::~Port()
{
    delete m_pSender;
}

void Port::Write(DMX512MessageShared spMsg, bool bAutoSend)
{
    m_spIncommingBatch->push_back(spMsg);
    if (bAutoSend)
    {
        TriggerSend();
    }
}

void Port::TriggerSend()
{
    if (m_spIncommingBatch->enough())
    {
        m_spOutcommingBatch.swap(m_spIncommingBatch);
        // Todo: Push Send command to Sender.
    }
}

void Port::TriggerCommit()
{
    // Todo: Push commit command to Sender.
}