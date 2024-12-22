#include "version.h"
#include "esp_log.h"
#include "cJSON.h"

void FWVersion::log()
{
    ESP_LOGI("FWVersion", "Version: %s", version);
    ESP_LOGI("FWVersion", "Build Time: %s", buildTime);
    ESP_LOGI("FWVersion", "Commit ID: %s", commitId);
}

cJSON *FWVersion::toJson()
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "Version", version);
    cJSON_AddStringToObject(json, "BuildTime", buildTime);
    cJSON_AddStringToObject(json, "CommitID", commitId);
    return json;
}