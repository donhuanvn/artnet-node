#ifndef __ARTNET_NODE_PORT_H__
#define __ARTNET_NODE_PORT_H__

#include <memory>
#include <queue>
#include "models/settings.h"
#include "miscellaneous.h"
#include <array>

class Sender;
class Batch;

class Port
{
public:
    enum ProtocolType
    {
        DMX,
        UCS1903
    };
    const int32_t m_s32PortNumber;

private:
    ProtocolType m_eType;
    Sender *m_pSender;
    std::shared_ptr<Batch> m_spOutcommingBatch;
    std::shared_ptr<Batch> m_spIncommingBatch;

public:
    Port(int32_t s32Number, ProtocolType eProtocol);
    ~Port();
    void Write(DMX512MessageShared spMsg, bool bAutoSend = true);
    void TriggerSend();
    void TriggerCommit();
};

#endif /* __ARTNET_NODE_PORT_H__ */
