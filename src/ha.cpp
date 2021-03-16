#include "ha.h"

static const char *TAG = "HomeAssistant";

uint8_t *HA::buffer = NULL;
esp_event_handler_instance_t HA::instance_lost_ip;
esp_event_handler_instance_t HA::instance_got_ip;
esp_mqtt_client_config_t *HA::mqttConfig = NULL;
esp_mqtt_client_handle_t HA::mqtt = NULL;
bool HA::unlockTaskRunning = false;

std::string HA::configTopic;
std::string HA::configRingingTopic;
std::string HA::configBatteryTopic;
std::string HA::stateTopic;
std::string HA::commandTopic;
std::string HA::ringingEntity;
std::string HA::batteryEntity;
std::string HA::currentVersion;
std::string HA::LWTMessage;

extern "C"
{
  int rom_phy_get_vdd33();
}

esp_err_t HA::init()
{
  buffer = new uint8_t[ELECTRA_ESP_HA_BUFFER_SIZE];

  currentVersion = Settings::getMqttUrl();
  currentVersion += ELECTRA_ESP_HA_VERSION;

  configTopic = "homeassistant/lock/";
  configTopic += Settings::getEntity();
  configTopic += "/lock/config";

  configRingingTopic = "homeassistant/binary_sensor/";
  configRingingTopic += Settings::getEntity();
  configRingingTopic += "/ringing/config";

  configBatteryTopic = "homeassistant/sensor/";
  configBatteryTopic += Settings::getEntity();
  configBatteryTopic += "/battery/config";

  stateTopic = "electra/";
  stateTopic += Settings::getEntity();

  commandTopic = stateTopic + "/set";

  ringingEntity = Settings::getEntity();
  ringingEntity += "_ringing";

  batteryEntity = Settings::getEntity();
  batteryEntity += "_battery";

  LWTMessage = "{\"state\":\"LOCK\",\"ringing\":false,\"battery\":";
  LWTMessage += std::to_string(Battery::getBatteryPercent());
  LWTMessage += "}";

  mqttConfig = new esp_mqtt_client_config_t();
  mqttConfig->uri = Settings::getMqttUrl();
  mqttConfig->username = Settings::getMqttUser();
  mqttConfig->password = Settings::getMqttPass();
  mqttConfig->keepalive = 5;
  mqttConfig->lwt_topic = stateTopic.c_str();
  mqttConfig->lwt_msg = LWTMessage.c_str();
  mqttConfig->lwt_qos = 1;
  mqttConfig->lwt_retain = 1;

  // add event handler for wifi connect/disconnect
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_LOST_IP,
                                                      &ip_event_handler,
                                                      NULL,
                                                      &instance_lost_ip));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &ip_event_handler,
                                                      NULL,
                                                      &instance_got_ip));
  return ESP_OK;
}

void HA::ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == IP_EVENT)
  {
    if (event_id == IP_EVENT_STA_GOT_IP)
    {
      ESP_LOGI(TAG, "Got IP");
      // create and connect to mqtt
      if (mqtt == NULL)
      {
        mqtt = esp_mqtt_client_init(mqttConfig);
        esp_mqtt_client_register_event(mqtt, MQTT_EVENT_ANY, mqtt_event_handler, mqtt);
        esp_mqtt_client_start(mqtt);
      }
    }
    else if (event_id == IP_EVENT_STA_LOST_IP)
    {
      ESP_LOGE(TAG, "Lost IP");
      // destroy mqtt client
      if (mqtt != NULL)
      {
        esp_mqtt_client_stop(mqtt);
        esp_mqtt_client_destroy(mqtt);
        mqtt = NULL;
      }
    }
  }
}

void HA::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  char *topic;
  char *data;
  switch (event->event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    if (setupRequired()) // only setup entities if not already done so
    {
      ESP_LOGI(TAG, "Updating HA configuration");
      setupEntity();
      setupStatusEntity();
      setupBatteryEntity();
      Settings::setHaVersion(currentVersion.c_str());
    }

    esp_mqtt_client_subscribe(mqtt, commandTopic.c_str(), 1);

    if (gpio_get_level(ESP_WAKEUP) == 1)
    {
      updateState();
    }
    else
    {
      updateState("LOCK", false);
    }
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    break;

  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    topic = new char[event->topic_len + 1];
    memcpy(topic, event->topic, event->topic_len);
    topic[event->topic_len] = '\0';
    if (strcmp(topic, commandTopic.c_str()) == 0)
    {
      data = new char[event->data_len + 1];
      memcpy(data, event->data, event->data_len);
      data[event->data_len] = '\0';
      if (strcmp(data, "UNLOCK") == 0)
      {
        if (!unlockTaskRunning)
        {
          unlockTaskRunning = true;
          ESP_LOGI(TAG, "Performing UNLOCK");
          // run unlock sequence in a separate thread so that mqtt status messages get sent to HA
          xTaskCreate(unlockTask, "unlock_task", 2048, NULL, 10, NULL);
        }
      }
    }
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    // if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS)
    // {
    //   log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
    //   log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
    //   log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
    //   ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
    // }
    break;
  default:
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }

  if (event->event_id != MQTT_EVENT_BEFORE_CONNECT)
  {
    Sleep::start();
  }
}

bool HA::setupRequired()
{
  char *storedVersion = Settings::getHaVersion();
  if (strcmp(storedVersion, currentVersion.c_str()) == 0)
  {
    return false;
  }
  return true;
}

void HA::createDevice(cJSON *&device)
{
  // device configuration, common for all entities
  device = cJSON_CreateObject();
  cJSON *name = cJSON_CreateString("Electra IA003");
  cJSON_AddItemToObject(device, "name", name);
  cJSON *manufacturer = cJSON_CreateString("kind3r");
  cJSON_AddItemToObject(device, "manufacturer", manufacturer);
  cJSON *model = cJSON_CreateString("ESP32");
  cJSON_AddItemToObject(device, "model", model);
  cJSON *sw_version = cJSON_CreateString("0.0.1");
  cJSON_AddItemToObject(device, "sw_version", sw_version);
  cJSON *identifiers = cJSON_CreateArray();
  cJSON_AddItemToObject(device, "identifiers", identifiers);
  cJSON *identifier = cJSON_CreateString(Settings::getEntity());
  cJSON_AddItemToArray(identifiers, identifier);
}

void HA::setupEntity()
{
  cJSON *device;
  createDevice(device);

  // entity configuration
  cJSON *entity = cJSON_CreateObject();
  cJSON *unique_id = cJSON_CreateString(Settings::getEntity());
  cJSON_AddItemToObject(entity, "unique_id", unique_id);
  cJSON *name2 = cJSON_CreateString("Electra IA003");
  cJSON_AddItemToObject(entity, "name", name2);
  cJSON_AddItemToObject(entity, "device", device);
  cJSON *state_topic = cJSON_CreateString(stateTopic.c_str());
  cJSON_AddItemToObject(entity, "state_topic", state_topic);
  cJSON *command_topic = cJSON_CreateString(commandTopic.c_str());
  cJSON_AddItemToObject(entity, "command_topic", command_topic);
  cJSON *payload_lock = cJSON_CreateString("LOCK");
  cJSON_AddItemToObject(entity, "payload_lock", payload_lock);
  cJSON *payload_unlock = cJSON_CreateString("UNLOCK");
  cJSON_AddItemToObject(entity, "payload_unlock", payload_unlock);
  cJSON *state_locked = cJSON_CreateString("LOCK");
  cJSON_AddItemToObject(entity, "state_locked", state_locked);
  cJSON *state_unlocked = cJSON_CreateString("UNLOCK");
  cJSON_AddItemToObject(entity, "state_unlocked", state_unlocked);
  cJSON *value_template = cJSON_CreateString("{{ value_json.state }}");
  cJSON_AddItemToObject(entity, "value_template", value_template);
  cJSON *optimistic = cJSON_CreateBool(false);
  cJSON_AddItemToObject(entity, "optimistic", optimistic);
  cJSON *retain = cJSON_CreateBool(false);
  cJSON_AddItemToObject(entity, "retain", retain);
  cJSON *platform = cJSON_CreateString("mqtt");
  cJSON_AddItemToObject(entity, "platform", platform);

  cJSON_PrintPreallocated(entity, (char *)buffer, ELECTRA_ESP_HA_BUFFER_SIZE, false);
  ESP_LOGI(TAG, "%s", buffer);

  int msg_id = esp_mqtt_client_publish(mqtt, configTopic.c_str(), (char *)buffer, 0, 1, 1);
  if (msg_id == -1)
  {
    ESP_LOGE(TAG, "Error sending entity config message");
  }

  cJSON_Delete(entity);
}

void HA::setupStatusEntity()
{
  cJSON *device;
  createDevice(device);

  cJSON *ringing = cJSON_CreateObject();
  cJSON *unique_id = cJSON_CreateString(ringingEntity.c_str());
  cJSON_AddItemToObject(ringing, "unique_id", unique_id);
  cJSON *name = cJSON_CreateString("Electra IA003 Ringing");
  cJSON_AddItemToObject(ringing, "name", name);
  cJSON_AddItemToObject(ringing, "device", device);
  cJSON *state_topic = cJSON_CreateString(stateTopic.c_str());
  cJSON_AddItemToObject(ringing, "state_topic", state_topic);
  cJSON *value_template = cJSON_CreateString("{{ value_json.ringing }}");
  cJSON_AddItemToObject(ringing, "value_template", value_template);
  cJSON *payload_on = cJSON_CreateBool(true);
  cJSON_AddItemToObject(ringing, "payload_on", payload_on);
  cJSON *payload_off = cJSON_CreateBool(false);
  cJSON_AddItemToObject(ringing, "payload_off", payload_off);
  cJSON *platform = cJSON_CreateString("mqtt");
  cJSON_AddItemToObject(ringing, "platform", platform);
  cJSON *json_attributes_topic = cJSON_CreateString(stateTopic.c_str());
  cJSON_AddItemToObject(ringing, "json_attributes_topic", json_attributes_topic);

  cJSON_PrintPreallocated(ringing, (char *)buffer, ELECTRA_ESP_HA_BUFFER_SIZE, false);
  ESP_LOGI(TAG, "%s", buffer);

  int msg_id = esp_mqtt_client_publish(mqtt, configRingingTopic.c_str(), (char *)buffer, 0, 1, 1);
  if (msg_id == -1)
  {
    ESP_LOGE(TAG, "Error sending entity status config message");
  }

  cJSON_Delete(ringing);
}

void HA::setupBatteryEntity()
{
  cJSON *device;
  createDevice(device);

  cJSON *battery = cJSON_CreateObject();
  cJSON *unique_id = cJSON_CreateString(batteryEntity.c_str());
  cJSON_AddItemToObject(battery, "unique_id", unique_id);
  cJSON *name = cJSON_CreateString("Electra IA003 Battery");
  cJSON_AddItemToObject(battery, "name", name);
  cJSON_AddItemToObject(battery, "device", device);
  cJSON *state_topic = cJSON_CreateString(stateTopic.c_str());
  cJSON_AddItemToObject(battery, "state_topic", state_topic);
  cJSON *value_template = cJSON_CreateString("{{ value_json.battery }}");
  cJSON_AddItemToObject(battery, "value_template", value_template);
  cJSON *device_class = cJSON_CreateString("battery");
  cJSON_AddItemToObject(battery, "device_class", device_class);
  cJSON *unit_of_measurement = cJSON_CreateString("%");
  cJSON_AddItemToObject(battery, "unit_of_measurement", unit_of_measurement);
  cJSON *platform = cJSON_CreateString("mqtt");
  cJSON_AddItemToObject(battery, "platform", platform);
  cJSON *json_attributes_topic = cJSON_CreateString(stateTopic.c_str());
  cJSON_AddItemToObject(battery, "json_attributes_topic", json_attributes_topic);

  cJSON_PrintPreallocated(battery, (char *)buffer, ELECTRA_ESP_HA_BUFFER_SIZE, false);
  ESP_LOGI(TAG, "%s", buffer);

  int msg_id = esp_mqtt_client_publish(mqtt, configBatteryTopic.c_str(), (char *)buffer, 0, 1, 1);
  if (msg_id == -1)
  {
    ESP_LOGE(TAG, "Error sending entity status config message");
  }

  cJSON_Delete(battery);
}

void HA::updateState(const char *lockState, bool lockRinging)
{
  cJSON *state = cJSON_CreateObject();
  cJSON *status = cJSON_CreateString(lockState);
  cJSON_AddItemToObject(state, "state", status);
  cJSON *ringing = cJSON_CreateBool(lockRinging);
  cJSON_AddItemToObject(state, "ringing", ringing);
  cJSON *battery = cJSON_CreateNumber(Battery::getBatteryPercent());
  cJSON_AddItemToObject(state, "battery", battery);

  cJSON_PrintPreallocated(state, (char *)buffer, ELECTRA_ESP_HA_BUFFER_SIZE, false);
  ESP_LOGI(TAG, "%s", buffer);

  int msg_id = esp_mqtt_client_publish(mqtt, stateTopic.c_str(), (char *)buffer, 0, 1, 1);
  if (msg_id == -1)
  {
    ESP_LOGE(TAG, "Error sending entity state update message");
  }

  cJSON_Delete(state);
}

void HA::unlockTask(void *arg)
{
  updateState("UNLOCK", false);
  Intercom::open();
  updateState("LOCK", false);
  unlockTaskRunning = false;
  vTaskDelete(NULL);
}