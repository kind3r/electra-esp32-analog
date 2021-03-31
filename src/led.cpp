#include "led.h"

static const char *TAG = "Led";

SemaphoreHandle_t Led::blinkSemaphore = NULL;
bool Led::blinkChanged;
blinkPattern_t *Led::blinkPattern;

void Led::init()
{
  // setup LED pin as output
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << ELECTRA_ESP_LED);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  gpio_set_level(ELECTRA_ESP_LED, 0);
  blinkChanged = false;
  blinkSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(blinkSemaphore);
  blinkPattern_t patern = PATTERN_BLINK_OFF();
  setBlinkPattern(&patern);

  xTaskCreate(ledTask, "led_task", 2048, NULL, 10, NULL);
}

void Led::setBlinkPattern(blinkPattern_t *patern)
{
  if (xSemaphoreTake(blinkSemaphore, 10) == pdTRUE)
  {
    blinkPattern = patern;
    blinkChanged = true;
    xSemaphoreGive(blinkSemaphore);
  }
  else
  {
    ESP_LOGI(TAG, "Failed to set blink pattern");
  }
}

void Led::ledTask(void *arg)
{
  blinkPattern_t *pattern = NULL;
  bool ledOn = false;
  for (;;)
  {
    if (xSemaphoreTake(blinkSemaphore, 10) == pdTRUE)
    {
      if (blinkChanged || pattern == NULL)
      {
        // change blink pattern
        pattern = blinkPattern;
        blinkChanged = false;
        gpio_set_level(ELECTRA_ESP_LED, 0);
        ledOn = false;
      }
      xSemaphoreGive(blinkSemaphore);
    }
    // do blink pattern
    for (uint8_t i = 0; i < pattern->cycles; i++)
    {
      gpio_set_level(ELECTRA_ESP_LED, ledOn ? 1 : 0);
      vTaskDelay(pattern->pattern[i] / portTICK_RATE_MS);
      ledOn = !ledOn;
    }
  }
}
