#ifndef ELECTRA_ESP_OTA_H
#define ELECTRA_ESP_OTA_H

#include <esp_log.h>
#include <esp_err.h>
#include <esp_ota_ops.h>
#include <esp_https_ota.h>
#include <esp_event.h>
#include <string.h>
#include "settings.h"
#include "led.h"

extern RTC_NOINIT_ATTR time_t lastOTACheck;

#define ELECTRA_ESP_OTA_URL "https://github.com/kind3r/electra-esp32-analog/raw/main/ota/firmware.bin"

class Ota
{
public:
  static void init();
  static char *getCurrentVersion();
  static bool getIsOTA();
private:
  static esp_app_desc_t currentApp;
  static bool OTACheck;
  static esp_event_handler_instance_t instance_got_ip;
  static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  static void otaTask(void *arg);
};

#endif