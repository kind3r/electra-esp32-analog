#include "sleep.h"

static const char *TAG = "Sleep";

bool Sleep::wasAwoken = false;

void Sleep::init()
{
  esp_sleep_enable_ext1_wakeup((1ULL << ESP_WAKEUP), ESP_EXT1_WAKEUP_ANY_HIGH);

  // setup wakeup pin as input
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << ESP_WAKEUP);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  if(gpio_get_level(ESP_WAKEUP) == 1) {
    wasAwoken = true;
  }

  xTaskCreate(gpioTask, "gpio_task", 2048, NULL, 10, NULL);
}

void Sleep::gpioTask(void *arg)
{
  for (;;)
  {
    if (gpio_get_level(ESP_WAKEUP) == 0) {
      // 5 second of LOW enters deep sleep
      vTaskDelay(5000 / portTICK_RATE_MS);
      if (gpio_get_level(ESP_WAKEUP) == 0) {
        ESP_LOGI(TAG, "Entering deep sleep");
        xTaskCreate(stopRingingTask, "stop_ringing", 2048, NULL, 10, NULL);
      }
    }
    vTaskDelay(500 / portTICK_RATE_MS);
  }
}

void Sleep::stopRingingTask(void *arg)
{
  HA::updateState("LOCK", false);
  uint32_t delay = 1000 / portTICK_RATE_MS;
  if (!wasAwoken) {
    // extra delay in case we were not awoken by ringing
    // we could also check the wake up reason to find out
    delay = 5000 / portTICK_RATE_MS;
  }
  vTaskDelay(delay);
  esp_deep_sleep_start();
  vTaskDelete(NULL); // won't really matter
}