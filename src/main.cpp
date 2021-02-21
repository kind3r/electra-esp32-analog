#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>
#include "settings.h"
#include "ha.h"
#include "wifi.h"
#include "util.h"

extern "C"
{
  void app_main(void);
}

void app_main()
{
  meminfo();
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  meminfo();

  if (Settings::init() != ESP_OK)
  {
    ESP_LOGE("main", "Error during Settings init");
    // panic ?
    for (;;)
    {
    }
  }
  else if (HA::init() != ESP_OK)
  {
    ESP_LOGE("main", "Error during HA init");
    // panic ?
    for (;;)
    {
    }
  }
  else if (WiFi::init() != ESP_OK)
  {
    ESP_LOGE("main", "Error during WiFi init");
    // panic ?
    for (;;)
    {
    }
  }

  meminfo();
}