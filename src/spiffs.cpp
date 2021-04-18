#include "spiffs.h"

static const char *TAG = "Spiffs";

bool Spiffs::ready = false;

esp_err_t Spiffs::init()
{
  if (ready)
  {
    return ESP_OK;
  }

  ESP_LOGV(TAG, "Mounting SPIFFS");

  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5, // This decides the maximum number of open files
      .format_if_mount_failed = true};

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS [%s]", esp_err_to_name(ret));
    }
    return ret;
  }

  ESP_LOGV(TAG, "SPIFFS mounted");

#if CONFIG_LOG_DEFAULT_LEVEL >= ESP_LOG_DEBUG
  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information [%s]", esp_err_to_name(ret));
    return false;
  }

  ESP_LOGD(TAG, "Partition size: total: %d, used: %d", total, used);
#endif

  ready = true;
  return ESP_OK;
}