#ifndef ELECTRA_ESP_SETTINGS_H
#define ELECTRA_ESP_SETTINGS_H

#ifndef ELECTRA_ESP_SETTINGS_FILE
#define ELECTRA_ESP_SETTINGS_FILE "/spiffs/config.json"
#endif

#ifndef ELECTRA_ESP_CONFIG_BUFFER_SIZE
#define ELECTRA_ESP_CONFIG_BUFFER_SIZE 3072
#endif

#include <string>
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
  static char *getMqttUrl();
  static char *getMqttUser();
  static char *getMqttPass();
  static char *getEntity();

private:
  static bool ready;
  static char *ssid;
  static char *pass;
  static char *mqttUrl;
  static char *mqttUser;
  static char *mqttPass;
  static char *entity;

  static esp_err_t initSPIFFS();
  static esp_err_t readConfig(uint8_t *buffer, size_t bufferLen);
  static esp_err_t getConfigVar(cJSON *root, const char *name, char *&var);
};

#endif
