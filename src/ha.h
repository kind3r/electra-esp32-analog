#ifndef ELECTRA_ESP_HA_H
#define ELECTRA_ESP_HA_H

#define ELECTRA_ESP_HA_BUFFER_SIZE 4096

#include <string>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_event.h>
#include <mqtt_client.h>
#include <cJSON.h>
#include "settings.h"
#include "intercom.h"

class HA
{
public:
  static esp_err_t init();

private:
  static uint8_t *buffer;
  static esp_event_handler_instance_t instance_lost_ip;
  static esp_event_handler_instance_t instance_got_ip;
  static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  static esp_mqtt_client_config_t *mqttConfig;
  static esp_mqtt_client_handle_t mqtt;
  static bool unlockTaskRunning;
  static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
  static std::string configTopic;
  static std::string configStatusTopic;
  static std::string stateTopic;
  static std::string commandTopic;
  static std::string statusEntity;
  static void setupEntity();
  static void setupStatusEntity();
  static void updateState(const char *lockState = "LOCK", bool lockRinging = true);
  static void unlockTask(void *arg);
};


#endif