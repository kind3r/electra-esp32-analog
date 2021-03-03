#include "sleep.h"

static const char *TAG = "Sleep";

bool Sleep::holdAwake = false;
bool Sleep::started = false;

void Sleep::init()
{
  esp_sleep_enable_ext1_wakeup((1ULL << ESP_WAKEUP | 1ULL << ESP_WAKE_HOLD), ESP_EXT1_WAKEUP_ANY_HIGH);

  // setup wakeup pin as input
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << ESP_WAKEUP | 1ULL << ESP_WAKE_HOLD);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  if (gpio_get_level(ESP_WAKE_HOLD) == 1)
  {
    holdAwake = true;
  }
}

void Sleep::start()
{
  if (!started && !holdAwake)
  {
    started = true;
    xTaskCreate(sleepTask, "sleep_task", 2048, NULL, 10, NULL);
    ESP_LOGI(TAG, "Monitor started");
  }
}

void Sleep::sleepTask(void *arg)
{
  for (;;)
  {
    if (gpio_get_level(ESP_WAKEUP) == 0)
    {
      // 1 second of LOW enters deep sleep
      vTaskDelay(1000 / portTICK_RATE_MS);
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
  HA::updateState("LOCK", false);
  uint32_t delay = 1000 / portTICK_RATE_MS;
  // if (!wasAwoken) {
  //   // extra delay in case we were not awoken by ringing
  //   // we could also check the wake up reason to find out
  //   delay = 5000 / portTICK_RATE_MS;
  // }
  vTaskDelay(delay);
  ESP_LOGI(TAG, "Entering deep sleep");
  esp_deep_sleep_start();
  vTaskDelete(NULL); // won't really matter
}