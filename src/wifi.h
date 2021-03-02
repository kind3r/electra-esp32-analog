#ifndef ELECTRA_ESP_WIFI_H
#define ELECTRA_ESP_WIFI_H

#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <freertos/event_groups.h>
#include "settings.h"

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define ESP_GW_WIFI_RETRY 5

class WiFi
{
public:
  static esp_err_t init();

private:
  static int s_retry_num;
  static EventGroupHandle_t s_wifi_event_group;
  static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  static bool wifi_init_sta(char *ssid, char *pass);
};

#endif