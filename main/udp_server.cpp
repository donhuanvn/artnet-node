#include "udp_server.h"
#include <esp_log.h>

const char * TAG = "UDP-Server";

void ArtNetServer::HandleIncommingMessage(size_t msgLength, char * senderIP)
{
    ESP_LOGI(TAG, "Receive ArtNet Message from '%s'", senderIP);
    ESP_LOGI(TAG, "Message length = %d", msgLength);
}

void CommonServer::HandleIncommingMessage(size_t msgLength, char * senderIP)
{
    ESP_LOGI(TAG, "Receive Common Message from '%s'", senderIP);
    ESP_LOGI(TAG, "Message length = %d", msgLength);
    ESP_LOGI(TAG, "Message '%s'", aRxBuffer.data());
}