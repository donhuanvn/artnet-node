#ifndef __UDP_SERVERS_H__
#define __UDP_SERVERS_H__

#include <stdio.h>
#include <array>
#include <functional>
#include "lwip/sockets.h"
#include "cJSON.h"

#ifndef UDP_ARTNET_BUFFER_LEN
#define UDP_ARTNET_BUFFER_LEN 535
#endif

#ifndef UDP_COMMON_BUFFER_LEN
#define UDP_COMMON_BUFFER_LEN 1024
#endif

typedef std::function<void(const char *, size_t, const char *)> MessageHandler_t;

class ArtNetServer
{
    std::array<char, UDP_ARTNET_BUFFER_LEN> m_aRxBuffer;
    MessageHandler_t m_oDMXHandler;
    MessageHandler_t m_oArtSyncHandler;
    MessageHandler_t m_oDiscoveryHandler;

    static int32_t m_s32Socket;
    static sockaddr_storage m_stSourceAddress; // Large enough for both IPv4 or IPv6

    void HandleIncommingMessage(size_t msgLength, char *senderIP);
    void CheckHandlers();
public:
    static ArtNetServer &GetInstance()
    {
        static ArtNetServer oIns;
        return oIns;
    }
    static void FreeRTOSTask(void *pvParamaters);
    std::array<char, UDP_ARTNET_BUFFER_LEN> &GetBufferRef() { return m_aRxBuffer; }
    inline char *GetBuffer() { return m_aRxBuffer.data(); }
    inline size_t GetBufferLength() { return m_aRxBuffer.size(); }
    void RegisterDMXMessageHandler(MessageHandler_t handler) { m_oDMXHandler = handler; }
    void RegisterArtSyncMessageHandler(MessageHandler_t handler) { m_oArtSyncHandler = handler; }
    void RegisterDiscoveryMessageHandler(MessageHandler_t handler) { m_oDiscoveryHandler = handler; }
    void Response(const char * pBuffer, size_t u32BufferSize);
};

class CommonServer
{
    std::array<char, UDP_COMMON_BUFFER_LEN> m_aRxBuffer;
    MessageHandler_t m_oMessageHandler;

    static int32_t m_s32Socket;
    static sockaddr_storage m_stSourceAddress; // Large enough for both IPv4 or IPv6

    void HandleIncommingMessage(size_t msgLength, char * senderIP);
    void CheckHandlers();

public:
    static CommonServer& GetInstance()
    {
        static CommonServer oIns;
        return oIns;
    }

    static void FreeRTOSTask(void *pvParamaters);
    std::array<char, UDP_COMMON_BUFFER_LEN> &GetBufferRef() { return m_aRxBuffer; }
    inline char * GetBuffer() {return m_aRxBuffer.data(); }
    inline size_t GetBufferLength() { return m_aRxBuffer.size(); }
    void RegisterMessageHandler(MessageHandler_t handler) { m_oMessageHandler = handler; }
    void Response(const char * pBuffer, size_t u32BufferSize);
};

#endif /* __UDP_SERVERS_H__ */
