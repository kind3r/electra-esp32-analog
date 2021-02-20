#ifndef ELECTRA_ESP_SETTINGS_H
#define ELECTRA_ESP_SETTINGS_H

#ifndef ELECTRA_ESP_SETTINGS_FILE
#define ELECTRA_ESP_SETTINGS_FILE "/spiffs/config.json"
#endif

#ifndef ELECTRA_ESP_CONFIG_BUFFER_SIZE
#define ELECTRA_ESP_CONFIG_BUFFER_SIZE 3072
#endif

#include <string.h>
#include <stdio.h>
#include <cJSON.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_spiffs.h>

class Settings
{
public:
  static esp_err_t init();
  static char *getSsid();
  static char *getPass();
  static char *getHa();

private:
  static bool ready;
  static char *ssid;
  static char *pass;
  static char *ha;

  static esp_err_t initSPIFFS();
  static esp_err_t readConfig(uint8_t *buffer, size_t bufferLen);
};

#endif
