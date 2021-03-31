#include "sleep.h"

static const char *TAG = "Sleep";

bool Sleep::started = false;
bool Sleep::wasRinging = false;
int Sleep::sleepDelay = 1;

void Sleep::init()
{
  switch (esp_sleep_get_wakeup_cause())
  {
  case ESP_SLEEP_WAKEUP_EXT0:
  case ESP_SLEEP_WAKEUP_EXT1:
    ESP_LOGI(TAG, "Wakeup: EXT");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    ESP_LOGI(TAG, "Wakeup: timer");
    break;
  case ESP_SLEEP_WAKEUP_UNDEFINED:
    ESP_LOGI(TAG, "Wakeup: undefined");
    break;
  case ESP_SLEEP_WAKEUP_ALL:
    ESP_LOGI(TAG, "Wakeup: all");
    break;
  default:
    ESP_LOGI(TAG, "Wakeup: default");
  }

  esp_sleep_enable_ext1_wakeup((1ULL << ESP_WAKEUP), ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_sleep_enable_timer_wakeup(1000000ULL * ESP_WAKEUP_INTERVAL);

  // setup wakeup pin as input
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << ESP_WAKEUP);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  if (gpio_get_level(ESP_WAKEUP) == 1)
  {
    Led::blinkFast();
    wasRinging = true;
  } else {
    wasRinging = false;
  }
}

void Sleep::start(int delay)
{
  if (!started)
  {
    started = true;
    sleepDelay = delay;
    xTaskCreate(sleepTask, "sleep_task", 2048, NULL, 10, NULL);
    ESP_LOGI(TAG, "Monitor started %d", delay);
  }
}

void Sleep::sleepTask(void *arg)
{
  for (;;)
  {
    if (gpio_get_level(ESP_WAKEUP) == 0)
    {
      if (wasRinging == true) {
        Led::blinkOff();
      }
      // 1 second of LOW enters deep sleep
      vTaskDelay(sleepDelay * 1000 / portTICK_RATE_MS);
      if (gpio_get_level(ESP_WAKEUP) == 0)
      {
        xTaskCreate(stopRingingTask, "stop_ringing", 2048, NULL, 10, NULL);
        vTaskDelete(NULL);
      }
    }
    vTaskDelay(500 / portTICK_RATE_MS);
  }
}

void Sleep::stopRingingTask(void *arg)
{
  // HA::updateState("LOCK", false);
  // uint32_t delay = 1000 / portTICK_RATE_MS;
  // vTaskDelay(delay);
  ESP_LOGI(TAG, "Entering deep sleep");
  esp_deep_sleep_start();
  vTaskDelete(NULL); // won't really matter
}