#include "udp_server.h"
#include <esp_log.h>
#include "config.h"

const char * TAG = "UDP-Server";

int32_t ArtNetServer::m_s32Socket = -1;
sockaddr_storage ArtNetServer::m_stSourceAddress;
int32_t CommonServer::m_s32Socket = -1;
sockaddr_storage CommonServer::m_stSourceAddress;

void ArtNetServer::HandleIncommingMessage(size_t msgLength, char * senderIP)
{
    if (msgLength < 10)
    {
        ESP_LOGD(TAG, "Receive Artnet Message with length less than 10");
        return;
    }
    int16_t op_code = m_aRxBuffer.data()[9] << 8 | m_aRxBuffer.data()[8];
    switch (op_code)
    {
    case 0x5000:
        m_oDMXHandler(m_aRxBuffer.data(), msgLength, senderIP);
        break;
    case 0x5200:
        m_oArtSyncHandler(m_aRxBuffer.data(), msgLength, senderIP);
        break;
    case 0x2009:
        m_oDiscoveryHandler(m_aRxBuffer.data(), msgLength, senderIP);
        break;
    default:
        ESP_LOGD(TAG, "Receive ArtNet Message with unsupported OPCODE '0x%04X'", op_code);
        break;
    }
}

void ArtNetServer::CheckHandlers()
{
    bool init = true;
    init &= (bool)m_oDMXHandler;
    init &= (bool)m_oArtSyncHandler;
    init &= (bool)m_oDiscoveryHandler;
    if (!init)
    {
        ESP_LOGE(TAG, "All handlers must be registered");
        ESP_ERROR_CHECK(ESP_ERR_NOT_FOUND);
    }
}

void ArtNetServer::FreeRTOSTask(void *pvParameters)
{
    ESP_LOGI(TAG, "UDP Server ArtNet Task starts");

    char addr_str[128];
    struct sockaddr_in dest_addr = {}; // IPv4
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PROJECT_UDP_ARTNET_PORT); // To read later: __builtin_bswap16

    ArtNetServer::GetInstance().CheckHandlers();

    while (true)
    {
        m_s32Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (m_s32Socket < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 3600;
        timeout.tv_usec = 0;
        setsockopt(m_s32Socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        int err = bind(m_s32Socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PROJECT_UDP_ARTNET_PORT);

        socklen_t socklen = sizeof(m_stSourceAddress);

        while (true)
        {
            char * buffer = ArtNetServer::GetInstance().GetBuffer();
            size_t bufferlen = ArtNetServer::GetInstance().GetBufferLength() - 1;
            int len = recvfrom(m_s32Socket, buffer, bufferlen, 0, (struct sockaddr *)&m_stSourceAddress, &socklen);
            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else if (m_stSourceAddress.ss_family == PF_INET)
            {
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&m_stSourceAddress)->sin_addr, addr_str, sizeof(addr_str) - 1);
                ArtNetServer::GetInstance().HandleIncommingMessage(len, addr_str);
            }
        }

        if (m_s32Socket != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(m_s32Socket, 0);
            close(m_s32Socket);
        }
    }
}

void ArtNetServer::Response(const char * pBuffer, size_t u32BufferSize)
{
    int err = sendto(m_s32Socket, pBuffer, u32BufferSize, 0, (struct sockaddr *)&m_stSourceAddress, sizeof(m_stSourceAddress));
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    }
}

void CommonServer::HandleIncommingMessage(size_t msgLength, char * senderIP)
{
    m_oMessageHandler(m_aRxBuffer.data(), msgLength, senderIP);
}

void CommonServer::CheckHandlers()
{
    bool init = true;
    init &= (bool)m_oMessageHandler;
    if (!init)
    {
        ESP_LOGE(TAG, "All handlers must be registered");
        ESP_ERROR_CHECK(ESP_ERR_NOT_FOUND);
    }
}

void CommonServer::FreeRTOSTask(void *pvParameters)
{
    ESP_LOGI(TAG, "UDP Server Common Task starts");

    char addr_str[128];
    struct sockaddr_in dest_addr = {}; // IPv4
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PROJECT_UDP_COMMON_PORT); // To read later: __builtin_bswap16

    CommonServer::GetInstance().CheckHandlers();

    while (true)
    {
        m_s32Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (m_s32Socket < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 3600;
        timeout.tv_usec = 0;
        setsockopt(m_s32Socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        int err = bind(m_s32Socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PROJECT_UDP_COMMON_PORT);

        socklen_t socklen = sizeof(m_stSourceAddress);

        while (true)
        {
            char * buffer = CommonServer::GetInstance().GetBuffer();
            size_t bufferlen = CommonServer::GetInstance().GetBufferLength() - 1;
            int len = recvfrom(m_s32Socket, buffer, bufferlen, 0, (struct sockaddr *)&m_stSourceAddress, &socklen);
            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else if (m_stSourceAddress.ss_family == PF_INET)
            {
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&m_stSourceAddress)->sin_addr, addr_str, sizeof(addr_str) - 1);
                CommonServer::GetInstance().HandleIncommingMessage(len, addr_str);
            }
        }

        if (m_s32Socket != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(m_s32Socket, 0);
            close(m_s32Socket);
        }
    }
}

void CommonServer::Response(const char * pBuffer, size_t u32BufferSize)
{
    int err = sendto(m_s32Socket, pBuffer, u32BufferSize, 0, (struct sockaddr *)&m_stSourceAddress, sizeof(m_stSourceAddress));
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    }
}
