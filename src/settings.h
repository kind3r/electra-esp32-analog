#ifndef ELECTRA_ESP_SETTINGS_H
#define ELECTRA_ESP_SETTINGS_H

#ifndef ELECTRA_ESP_SETTINGS_FILE
#define ELECTRA_ESP_SETTINGS_FILE "/spiffs/config.json"
#endif

#ifndef ELECTRA_ESP_CONFIG_BUFFER_SIZE
#define ELECTRA_ESP_CONFIG_BUFFER_SIZE 3072
#endif

#define ELECTRA_ESP_DEFAULT_CONFIG "{\"entity\": \"electra32\"}"

#include <string>
#include <string.h>
#include <stdio.h>
#include <cJSON.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_spiffs.h>
#include <nvs.h>
#include <nvs_flash.h>

typedef enum {
    PT_I8, PT_U8, PT_I16, PT_U16, PT_I32, PT_U32, PT_I64, PT_U64, PT_STR, PT_BLOB, PT_INVALID
} PreferenceType;

RTC_NOINIT_ATTR bool forceSetupMode = false;

class Settings
{
public:
  static esp_err_t init();
  static char *getHaVersion();
  static void setHaVersion(const char *newVersion);
  static char *getSsid();
  static char *getPass();
  static char *getMqttUrl();
  static char *getMqttUser();
  static char *getMqttPass();
  static char *getEntity();
  static size_t getConfigFile(uint8_t *buffer);
  static esp_err_t putConfigFile(uint8_t *buffer, size_t bufferLen);

private:
  static bool ready;
  static uint32_t handle;
  static char *haVersion;
  static char *ssid;
  static char *pass;
  static char *mqttUrl;
  static char *mqttUser;
  static char *mqttPass;
  static char *entity;

  static esp_err_t initSPIFFS();
  static esp_err_t readConfig(uint8_t *buffer, size_t bufferLen);
  static esp_err_t getConfigVar(cJSON *root, const char *name, char *&var);
  static bool isKey(const char* key);
  static PreferenceType getType(const char* key);
  static size_t putBytes(const char* key, const void* value, size_t len);
  static size_t getBytesLength(const char* key);
  static size_t getBytes(const char* key, void * buf, size_t maxLen);
};

#endif
