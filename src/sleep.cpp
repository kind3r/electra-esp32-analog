#include "sleep.h"

static const char *TAG = "Sleep";

bool Sleep::started = false;
bool Sleep::wasRinging = false;
int Sleep::sleepDelay = 1;

void Sleep::init()
{
  esp_sleep_enable_ext1_wakeup((1ULL << ELECTRA_ESP_RINGING), ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_sleep_enable_timer_wakeup(1000000ULL * ESP_WAKEUP_INTERVAL);

  if (Intercom::isRinging())
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
    if (!Intercom::isRinging())
    {
      if (wasRinging == true) {
        Led::blinkOff();
      }
      // 1 second of LOW enters deep sleep
      vTaskDelay(sleepDelay * 1000 / portTICK_RATE_MS);
      if (!Intercom::isRinging())
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
  esp_wifi_stop();
  esp_deep_sleep_start();
  vTaskDelete(NULL); // won't really matter
}