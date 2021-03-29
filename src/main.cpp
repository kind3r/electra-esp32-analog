#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>
#include "settings.h"
#include "ha.h"
#include "wifi.h"
#include "wifiap.h"
#include "util.h"
#include "intercom.h"
#include "sleep.h"
#include "battery.h"

extern "C"
{
  void app_main(void);
}

void app_main()
{
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  Battery::init();

  Sleep::init();

  Intercom::init();

  if (Settings::init() != ESP_OK)
  {
    ESP_LOGE("main", "Error during Settings init");
    // enter config mode
    WiFiAP::init();
    Sleep::start(300); // sleep after 5 minutes
  }
  else if (HA::init() != ESP_OK)
  {
    ESP_LOGE("main", "Error during HA init");
    Sleep::start();
  }
  else if (WiFi::init() != ESP_OK)
  {
    ESP_LOGE("main", "Error during WiFi init");
    Sleep::start();
  }
}