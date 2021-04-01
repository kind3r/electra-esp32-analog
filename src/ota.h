#ifndef ELECTRA_ESP_OTA_H
#define ELECTRA_ESP_OTA_H

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <string.h>

class Ota
{
public:
  static void init();
  static char *getCurrentVersion();
private:
  static esp_app_desc_t currentApp;
};

#endif