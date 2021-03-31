#include "led.h"

static const char *TAG = "Led";

uint16_t blinkPatternOff[2] = {0, 100};
uint16_t blinkPatternSlow[2] = {200, 1000};
uint16_t blinkPatternFast[2] = {100, 100};

SemaphoreHandle_t Led::blinkSemaphore = NULL;
bool Led::blinkChanged;
uint16_t *Led::blinkPattern = NULL;
size_t Led::blinkPatternLen = 0;

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
  blinkSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(blinkSemaphore);
  setBlinkPattern(blinkPatternOff, 2);
  blinkChanged = false;

  xTaskCreate(ledTask, "led_task", 2048, NULL, 10, NULL);
}

void Led::blinkOff()
{
  setBlinkPattern(blinkPatternOff, 2);
}

void Led::blinkSlow()
{
  setBlinkPattern(blinkPatternSlow, 2);
}

void Led::blinkFast()
{
  setBlinkPattern(blinkPatternFast, 2);
}

void Led::setBlinkPattern(uint16_t *pattern, size_t patternLen)
{
  if (xSemaphoreTake(blinkSemaphore, portMAX_DELAY) == pdTRUE)
  {
    blinkPatternLen = patternLen;
    if (blinkPattern != NULL)
    {
      delete[] blinkPattern;
    }
    blinkPattern = new uint16_t[patternLen];
    for (size_t i = 0; i < patternLen; i++)
    {
      blinkPattern[i] = pattern[i];
    }
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
  size_t patternLen = blinkPatternLen;
  uint16_t *pattern;
  pattern = new uint16_t[blinkPatternLen];
  for (size_t i = 0; i < patternLen; i++)
  {
    pattern[i] = blinkPattern[i];
  }
  bool ledOn = false;
  for (;;)
  {
    if (xSemaphoreTake(blinkSemaphore, 10) == pdTRUE)
    {
      if (blinkChanged)
      {
        // change blink pattern
        patternLen = blinkPatternLen;
        delete[] pattern;
        pattern = new uint16_t[blinkPatternLen];
        for (size_t i = 0; i < patternLen; i++)
        {
          pattern[i] = blinkPattern[i];
        }
        blinkChanged = false;
        gpio_set_level(ELECTRA_ESP_LED, 0);
        ledOn = false;
      }
      xSemaphoreGive(blinkSemaphore);
    }
    // do blink pattern
    for (size_t i = 0; i < patternLen; i++)
    {
      ledOn = !ledOn;
      gpio_set_level(ELECTRA_ESP_LED, ledOn ? 1 : 0);
      if (pattern[i] > 0)
      {
        vTaskDelay(pattern[i] / portTICK_RATE_MS);
      }
    }
  }
}
