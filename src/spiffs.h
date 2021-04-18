#ifndef ELECTRA_ESP_SPIFFS_H
#define ELECTRA_ESP_SPIFFS_H

#include <esp_log.h>
#include <esp_err.h>
#include <esp_spiffs.h>

class Spiffs
{
public:
  static esp_err_t init();
private:
  static bool ready;
};

#endif