#include "ota.h"

static const char *TAG = "OTA";

time_t lastOTACheck = 0;

esp_app_desc_t Ota::currentApp;
bool Ota::OTACheck = false;
esp_event_handler_instance_t Ota::instance_got_ip;

void Ota::init()
{
  const esp_partition_t *current_partition = esp_ota_get_running_partition();
  esp_ota_get_partition_description(current_partition, &currentApp);

  /**
   * OTA Check Logic
   * - save in nvs the time of the last check
   * - compare current time with last check time
   * - if last check time > current time or difference is more than 1 day, check for new version is required
   * - if check new version is required, bind to wifi connect event and launch a task
   */
  time_t now;
  time(&now);
  double diff = difftime(now, lastOTACheck);
  ESP_LOGI(TAG, "Last check: %ld Now: %ld Diff: %f", lastOTACheck, now, diff);
  if (diff <= 0 || diff > 60 * 60 * 24)
  {
    ESP_LOGI(TAG, "Check required %s", currentApp.version);
    OTACheck = true;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &ip_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
  } else {
    ESP_LOGI(TAG, "Check not required");
  }
}

char *Ota::getCurrentVersion()
{
  return currentApp.version;
}

bool Ota::getIsOTA()
{
  return OTACheck;
}

void Ota::ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    xTaskCreate(otaTask, "OTA", 8092, NULL, 10, NULL);
  }
}

void Ota::otaTask(void *arg)
{
  /** 
   * OTA Check Task
   * - mark sleep task as being busy so it does not sleep
   * - check if new version is available
   * - if not, release busy of sleep task
   * - else, perform OTA and restart
   */
  esp_http_client_config_t httpConfig = {
      .url = ELECTRA_ESP_OTA_URL,
  };
  esp_https_ota_config_t otaConfig = {
      .http_config = &httpConfig,
  };
  esp_https_ota_handle_t otaHandle = NULL;

  if (esp_https_ota_begin(&otaConfig, &otaHandle) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed ota_begin");
  }
  else
  {
    esp_app_desc_t newApp;
    if (esp_https_ota_get_img_desc(otaHandle, &newApp) != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed getting image description");
    }
    else
    {
      if (memcmp(newApp.version, currentApp.version, sizeof(newApp.version)) == 0)
      {
        time(&lastOTACheck);
        ESP_LOGI(TAG, "No OTA update required %s", newApp.version);
      }
      else
      {
        ESP_LOGI(TAG, "OTA update from %s to %s", currentApp.version, newApp.version);
        Led::blinkOn();
        while (esp_https_ota_perform(otaHandle) == ESP_ERR_HTTPS_OTA_IN_PROGRESS)
          ;

        if (esp_https_ota_is_complete_data_received(otaHandle) != true)
        {
          ESP_LOGE(TAG, "Failed OTA download");
        } else {
          if (esp_https_ota_finish(otaHandle) == ESP_OK)
          {
            time(&lastOTACheck);
            ESP_LOGI(TAG, "Rebooting...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
          } else {
            ESP_LOGE(TAG, "Failed applying OTA");
          }
        }
      }
    }
  }

  OTACheck = false;
  vTaskDelete(NULL);
}