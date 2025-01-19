#include "Arduino.h"
#include "version.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "config.h"
#include "udp_server.h"
#include "miscellaneous.h"
#include "models/settings.h"
#include "models/info.h"
#include "models/status.h"
#include "port.h"

static const char *TAG = "Main";

static void hello_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Hello Task starts");
    while(true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Free Heap %d Kbytes", heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) >> 10);
        Status::GetInstance().Log();
        static bool isPrinted = false;
        if (!isPrinted)
        {
            isPrinted = true;
            Settings::GetInstance().Log();
        }
        vTaskDelay(9000 / portTICK_PERIOD_MS);
    }
}

static void dmx_message_handler(const char * msg, size_t len, const char * sender)
{
    // ESP_LOGI(TAG, "dmx_message_handler with size of %d - from %s", (int)len, sender);
    std::shared_ptr<DMX512Message> spMessage = std::make_shared<DMX512Message>();
    memcpy(spMessage->GetBuffer(), msg, len);
    int32_t s32StartUniv = Settings::GetInstance().GetStartUniverse();
    int32_t s32EndUniv = s32StartUniv + Settings::GetInstance().GetNoUniverses();
    if (s32StartUniv <= spMessage->GetUniverse() && spMessage->GetUniverse() < s32EndUniv)
    {
        Ports::GetInstance().HandleDMXMessage(spMessage);
        Status::GetInstance().UpdateForNewDMXMessage(spMessage->GetUniverse());
    }
}

static void artsync_message_handler(const char * msg, size_t len, const char * sender)
{
    // ESP_LOGI(TAG, "artsync_message_handler");
    Ports::GetInstance().Sync();
}

static void discovery_message_handler(const char * msg, size_t len, const char * sender)
{
    ESP_LOGI(TAG, "discovery_message_handler");
    // Todo: return information to the source, update the host ip.
}

static void common_message_handler(const char * msg, size_t len, const char * sender)
{
    ESP_LOGI(TAG, "common_message_handler");
    // Todo: factory reset, setting, query settings, query status, query info
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ArtNet Node - Full Master Wireless");
    FWVersion::log();

    initArduino();
    Serial.begin(115200);
    while(!Serial){
        ; // wait for serial port to connect
    }

    // ESP_ERROR_CHECK(nvs_flash_erase());
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    HWStatus::Mode mode = HWStatus::GetMode();
    if (mode == HWStatus::Mode::CONFIG_ONLY)
    {
        WifiAP::GetInstance().Init();
        CommonServer::GetInstance().RegisterMessageHandler(common_message_handler);
        xTaskCreate(CommonServer::FreeRTOSTask, "CommonServer::FreeRTOSTask", 4096, NULL, configMAX_PRIORITIES - 2, NULL);
    }
    else if (mode == HWStatus::Mode::CONFIG_AND_RUN_DMX)
    {
        WifiSTA::GetInstance().Init();

        ArtNetServer::GetInstance().RegisterDMXMessageHandler(dmx_message_handler);
        ArtNetServer::GetInstance().RegisterArtSyncMessageHandler(artsync_message_handler);
        ArtNetServer::GetInstance().RegisterDiscoveryMessageHandler(discovery_message_handler);
        CommonServer::GetInstance().RegisterMessageHandler(common_message_handler);

        Ports::GetInstance().Init();

        xTaskCreate(ArtNetServer::FreeRTOSTask, "ArtNetServer::FreeRTOSTask", 4096, NULL, configMAX_PRIORITIES - 2, NULL);
        xTaskCreate(CommonServer::FreeRTOSTask, "CommonServer::FreeRTOSTask", 4096, NULL, configMAX_PRIORITIES - 3, NULL);
        xTaskCreate(Ports::FreeRTOSTask, "Ports::FreeRTOSTask", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    }

    // static const char * pSettings = "{\"BroadcastSSID\":\"ESP_D0FC29\",\"BroadcastPassword\":\"\",\"SiteSSID\":\"P102\",\"SitePassword\":\"123567890\",\"StaticIP\":\"\",\"LedType\":\"\",\"TimeHigh\":-1,\"TimeLow\":-1,\"StartUniverse\":0,\"NoUniverses\":24,\"Identity\":\"\",\"Model\":\"\",\"ProductID\":\"\",\"ArtNetSync\":false,\"Ports\":[{\"StartUniverse\":0,\"NoUniverses\":6,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":6,\"NoUniverses\":6,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"}]}";
    // cJSON * json = cJSON_Parse(pSettings);
    // Settings::GetInstance().FromJson(json);
    // cJSON_Delete(json);

    xTaskCreate(hello_task, "hello_task", 4096, NULL, configMAX_PRIORITIES - 10, NULL);
}
