#include "ha.h"

static const char *TAG = "HomeAssistant";

uint8_t *HA::buffer = NULL;
esp_event_handler_instance_t HA::instance_lost_ip;
esp_event_handler_instance_t HA::instance_got_ip;
esp_mqtt_client_config_t *HA::mqttConfig = NULL;
esp_mqtt_client_handle_t HA::mqtt = NULL;

esp_err_t HA::init()
{
  buffer = new uint8_t[ELECTRA_ESP_HA_BUFFER_SIZE];
  mqttConfig = new esp_mqtt_client_config_t();
  mqttConfig->uri = Settings::getMqttUrl();
  mqttConfig->username = Settings::getMqttUser();
  mqttConfig->password = Settings::getMqttPass();

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
      if (mqtt != NULL) {
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
  switch (event->event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
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
    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    printf("DATA=%.*s\r\n", event->data_len, event->data);
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