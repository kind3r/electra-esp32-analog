#ifndef ELECTRA_ESP_WIFIAP_H
#define ELECTRA_ESP_WIFIAP_H

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <mdns.h>
#include <cJSON.h>

#include "settings.h"
#include "led.h"
#include "log.h"

#define ELECTRA_ESP_WIFI_CONFIG_SSID "ELECTRA32"
#define ELECTRA_ESP_WIFI_CONFIG_PASS "87654321"
#define ELECTRA_ESP_WIFI_CONFIG_CHANNEL 6
#define ELECTRA_ESP_WIFI_CONFIG_MAX_CONNECTIONS 2
#define ELECTRA_ESP_WIFI_HOSTNAME "electra32"

class WiFiAP
{
public:
  static void init();

private:
  static httpd_handle_t server;
  static httpd_uri_t home;
  static httpd_uri_t configGet;
  static httpd_uri_t configSet;
  static httpd_uri_t logsGet;
  static httpd_uri_t logGet;
  static uint8_t *buffer;
  static logFiles_t logs;
  static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  static void wifi_init_softap();
  static void mdns_start();
  static void startWebServer();
  static esp_err_t handleHome(httpd_req_t *req);
  static esp_err_t handleConfigGet(httpd_req_t *req);
  static esp_err_t handleConfigSet(httpd_req_t *req);
  static esp_err_t handleLogsGet(httpd_req_t *req);
  static esp_err_t handleLogGet(httpd_req_t *req);
  static void rebootTask(void *arg);
};

#endif