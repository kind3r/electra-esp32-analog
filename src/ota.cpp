#include "ota.h"

static const char *TAG = "OTA";

esp_app_desc_t Ota::currentApp;

void Ota::init() {
  const esp_partition_t *current_partition = esp_ota_get_running_partition();
  esp_ota_get_partition_description(current_partition, &currentApp);
}

char *Ota::getCurrentVersion()
{
  return currentApp.version;
}