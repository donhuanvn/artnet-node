#ifndef __UDP_SERVERS_H__
#define __UDP_SERVERS_H__

#include <stdio.h>
#include <array>


#ifndef UDP_ARTNET_BUFFER_LEN
#define UDP_ARTNET_BUFFER_LEN 535
#endif

#ifndef UDP_COMMON_BUFFER_LEN
#define UDP_COMMON_BUFFER_LEN 1024
#endif

class ArtNetServer
{
    std::array<char, UDP_ARTNET_BUFFER_LEN> aRxBuffer;

public:
    static ArtNetServer& GetInstance()
    {
        static ArtNetServer oIns;
        return oIns;
    }

    std::array<char, UDP_ARTNET_BUFFER_LEN> &GetBufferRef() { return aRxBuffer; }
    inline char * GetBuffer() {return aRxBuffer.data(); }
    inline size_t GetBufferLength() { return aRxBuffer.size(); }
    void HandleIncommingMessage(size_t msgLength, char * senderIP);
};

class CommonServer
{
    std::array<char, UDP_COMMON_BUFFER_LEN> aRxBuffer;

public:
    static CommonServer& GetInstance()
    {
        static CommonServer oIns;
        return oIns;
    }

    std::array<char, UDP_COMMON_BUFFER_LEN> &GetBufferRef() { return aRxBuffer; }
    inline char * GetBuffer() {return aRxBuffer.data(); }
    inline size_t GetBufferLength() { return aRxBuffer.size(); }
    void HandleIncommingMessage(size_t msgLength, char * senderIP);
};

#endif /* __UDP_SERVERS_H__ */
