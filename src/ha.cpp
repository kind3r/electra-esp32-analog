#include "ha.h"

static const char *TAG = "HomeAssistant";
static const char *LWTMessage = "{\"state\":\"LOCK\",\"ringing\":false}";

uint8_t *HA::buffer = NULL;
esp_event_handler_instance_t HA::instance_lost_ip;
esp_event_handler_instance_t HA::instance_got_ip;
esp_mqtt_client_config_t *HA::mqttConfig = NULL;
esp_mqtt_client_handle_t HA::mqtt = NULL;

std::string HA::configTopic;
std::string HA::configStatusTopic;
std::string HA::stateTopic;
std::string HA::commandTopic;
std::string HA::statusEntity;

esp_err_t HA::init()
{
  buffer = new uint8_t[ELECTRA_ESP_HA_BUFFER_SIZE];

  configTopic = "homeassistant/lock/";
  configTopic += Settings::getEntity();
  configTopic += "/lock/config";

  configStatusTopic = "homeassistant/binary_sensor/";
  configStatusTopic += Settings::getEntity();
  configStatusTopic += "/status/config";

  stateTopic = "electra/";
  stateTopic += Settings::getEntity();

  commandTopic = stateTopic + "/set";

  statusEntity = Settings::getEntity();
  statusEntity += "_status";

  mqttConfig = new esp_mqtt_client_config_t();
  mqttConfig->uri = Settings::getMqttUrl();
  mqttConfig->username = Settings::getMqttUser();
  mqttConfig->password = Settings::getMqttPass();
  mqttConfig->keepalive = 5;
  mqttConfig->lwt_topic = stateTopic.c_str();
  mqttConfig->lwt_msg = LWTMessage;
  // mqttConfig->lwt_msg_len = strlen(LWTMessage) + 1;
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
    // TODO: entity setup should only be done once, maybe store in nvs
    // if config was done so we don't waste the time
    setupEntity();
    setupStatusEntity();

    esp_mqtt_client_subscribe(mqtt, commandTopic.c_str(), 1);

    updateState();
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
        ESP_LOGI(TAG, "Performing UNLOCK");
        updateState("UNLOCK", false);
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
}

void HA::setupEntity()
{
  // device configuration
  cJSON *device = cJSON_CreateObject();
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
  // device configuration
  cJSON *device = cJSON_CreateObject();
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

  cJSON *status = cJSON_CreateObject();
  cJSON *unique_id = cJSON_CreateString(statusEntity.c_str());
  cJSON_AddItemToObject(status, "unique_id", unique_id);
  cJSON *name2 = cJSON_CreateString("Electra IA003 Ringing");
  cJSON_AddItemToObject(status, "name", name2);
  cJSON_AddItemToObject(status, "device", device);
  cJSON *state_topic = cJSON_CreateString(stateTopic.c_str());
  cJSON_AddItemToObject(status, "state_topic", state_topic);
  cJSON *value_template2 = cJSON_CreateString("{{ value_json.ringing }}");
  cJSON_AddItemToObject(status, "value_template", value_template2);
  cJSON *payload_on = cJSON_CreateBool(true);
  cJSON_AddItemToObject(status, "payload_on", payload_on);
  cJSON *payload_off = cJSON_CreateBool(false);
  cJSON_AddItemToObject(status, "payload_off", payload_off);
  cJSON *icon = cJSON_CreateString("mdi:bell");
  cJSON_AddItemToObject(status, "icon", icon);

  cJSON_PrintPreallocated(status, (char *)buffer, ELECTRA_ESP_HA_BUFFER_SIZE, false);
  ESP_LOGI(TAG, "%s", buffer);

  int msg_id = esp_mqtt_client_publish(mqtt, configStatusTopic.c_str(), (char *)buffer, 0, 1, 1);
  if (msg_id == -1)
  {
    ESP_LOGE(TAG, "Error sending entity status config message");
  }

  cJSON_Delete(status);
}

void HA::updateState(const char *lockState, bool lockRinging)
{
  cJSON *state = cJSON_CreateObject();
  cJSON *status = cJSON_CreateString(lockState);
  cJSON_AddItemToObject(state, "state", status);
  cJSON *ringing = cJSON_CreateBool(lockRinging);
  cJSON_AddItemToObject(state, "ringing", ringing);

  cJSON_PrintPreallocated(state, (char *)buffer, ELECTRA_ESP_HA_BUFFER_SIZE, false);
  ESP_LOGI(TAG, "%s", buffer);

  int msg_id = esp_mqtt_client_publish(mqtt, stateTopic.c_str(), (char *)buffer, 0, 1, 1);
  if (msg_id == -1)
  {
    ESP_LOGE(TAG, "Error sending entity state update message");
  }

  cJSON_Delete(state);
}
