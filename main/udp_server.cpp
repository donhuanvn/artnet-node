#include "udp_server.h"
#include <esp_log.h>
#include "config.h"
#include "lwip/sockets.h"

const char * TAG = "UDP-Server";

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
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 3600;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PROJECT_UDP_ARTNET_PORT);

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);

        while (true)
        {
            char * buffer = ArtNetServer::GetInstance().GetBuffer();
            size_t bufferlen = ArtNetServer::GetInstance().GetBufferLength() - 1;
            int len = recvfrom(sock, buffer, bufferlen, 0, (struct sockaddr *)&source_addr, &socklen);
            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else if (source_addr.ss_family == PF_INET)
            {
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                ArtNetServer::GetInstance().HandleIncommingMessage(len, addr_str);
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
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
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 3600;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PROJECT_UDP_COMMON_PORT);

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);

        while (true)
        {
            char * buffer = CommonServer::GetInstance().GetBuffer();
            size_t bufferlen = CommonServer::GetInstance().GetBufferLength() - 1;
            int len = recvfrom(sock, buffer, bufferlen, 0, (struct sockaddr *)&source_addr, &socklen);
            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else if (source_addr.ss_family == PF_INET)
            {
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                CommonServer::GetInstance().HandleIncommingMessage(len, addr_str);
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
}
