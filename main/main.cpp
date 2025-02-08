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

static void esp_restart_handle(TimerHandle_t xTimer)
{
    esp_restart();
}

static TimerHandle_t esp_restart_timer = xTimerCreate("esp_restart_timer", pdMS_TO_TICKS(2000), pdFALSE, NULL, &esp_restart_handle);

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
    InfoModel::GetInstance().SetHostAppIP(sender);
    char * pResponse = cJSON_PrintUnformatted(InfoModel::GetInstance().ToJson());
    ArtNetServer::GetInstance().Response(pResponse, strlen(pResponse));
    delete pResponse;
}

static void common_message_handler(const char * msg, size_t len, const char * sender)
{
    ESP_LOGI(TAG, "common_message_handler");
    // Todo: factory reset, setting, query settings, query status, query info
    ESP_LOGI(TAG, "Message '%s'", msg);

    cJSON * pRequest = cJSON_ParseWithLength(msg, len);
    cJSON * pResponse = cJSON_CreateObject();
    do
    {
        if (!cJSON_IsObject(pRequest))
        {
            cJSON_AddStringToObject(pResponse, "message", "Invalid request");
            cJSON_AddNumberToObject(pResponse, "error_code", 400);
            break;
        }

        cJSON * pAction = cJSON_GetObjectItemCaseSensitive(pRequest, "action");
        if (!cJSON_IsString(pAction))
        {
            cJSON_AddStringToObject(pResponse, "message", "Invalid request");
            cJSON_AddNumberToObject(pResponse, "error_code", 400);
            break;
        }

        std::string sAction(pAction->valuestring);
        if (sAction == "factory_reset")
        {
            ESP_LOGI(TAG, "factory_reset");
            ESP_ERROR_CHECK(nvs_flash_erase());
            cJSON_AddStringToObject(pResponse, "message", "Factory reset done");
            cJSON_AddNumberToObject(pResponse, "error_code", 200);
            xTimerStart(esp_restart_timer, portMAX_DELAY);
        }
        else if (sAction == "update_setting")
        {
            ESP_LOGI(TAG, "update_setting");
            cJSON * pData = cJSON_GetObjectItemCaseSensitive(pRequest, "data");
            if (!cJSON_IsObject(pData))
            {
                cJSON_AddStringToObject(pResponse, "message", "Wrong data");
                cJSON_AddNumberToObject(pResponse, "error_code", 400);
                break;
            }
            Settings::GetInstance().FromJson(pData);
            cJSON_AddStringToObject(pResponse, "message", "Update setting done");
            cJSON_AddNumberToObject(pResponse, "error_code", 200);
            cJSON_AddItemToObject(pResponse, "data", Settings::GetInstance().ToJson());
        }
        else if (sAction == "read_setting")
        {
            cJSON_AddStringToObject(pResponse, "message", "Read setting done");
            cJSON_AddNumberToObject(pResponse, "error_code", 200);
            cJSON_AddItemToObject(pResponse, "data", Settings::GetInstance().ToJson());
        }
        else if (sAction == "read_status")
        {
            cJSON_AddStringToObject(pResponse, "message", "Read status done");
            cJSON_AddNumberToObject(pResponse, "error_code", 200);
            cJSON_AddItemToObject(pResponse, "data", Status::GetInstance().ToJson());
        }
        else if (sAction == "read_info")
        {
            cJSON_AddStringToObject(pResponse, "message", "Read info done");
            cJSON_AddNumberToObject(pResponse, "error_code", 200);
            cJSON_AddItemToObject(pResponse, "data", InfoModel::GetInstance().ToJson());
        }
        else
        {
            cJSON_AddStringToObject(pResponse, "message", "Invalid action");
            cJSON_AddNumberToObject(pResponse, "error_code", 400);
            break;
        }
    } while (false);
    
    char * pMsg = cJSON_PrintUnformatted(pResponse);
    CommonServer::GetInstance().Response(pMsg, strlen(pMsg));
    delete pRequest;
    delete pResponse;
    delete pMsg;
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

    // static const char * pSettings = "{\"BroadcastSSID\":\"ESP_D0FC29\",\"BroadcastPassword\":\"\",\"SiteSSID\":\"Bo home-Ext\",\"SitePassword\":\"namnamnam\",\"StaticIP\":\"\",\"LedType\":\"\",\"TimeHigh\":-1,\"TimeLow\":-1,\"StartUniverse\":0,\"NoUniverses\":24,\"Identity\":\"\",\"Model\":\"\",\"ProductID\":\"\",\"ArtNetSync\":false,\"Ports\":[{\"StartUniverse\":0,\"NoUniverses\":6,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":6,\"NoUniverses\":6,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"},{\"StartUniverse\":-1,\"NoUniverses\":-1,\"LedCount\":1020,\"LedType\":\"SM16703\"}]}";
    // cJSON * json = cJSON_Parse(pSettings);
    // Settings::GetInstance().FromJson(json);
    // cJSON_Delete(json);

    xTaskCreate(hello_task, "hello_task", 4096, NULL, configMAX_PRIORITIES - 10, NULL);
}
