#ifndef ELECTRA_ESP_LED_H
#define ELECTRA_ESP_LED_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/gpio.h>
#include <esp_log.h>

#define ELECTRA_ESP_LED GPIO_NUM_2

class Led
{
public:
  static void init();
  static void blinkOff();
  static void blinkSlow();
  static void blinkFast();
  static void setBlinkPattern(uint16_t *pattern, size_t patternLen);
private:
  static SemaphoreHandle_t blinkSemaphore;
  static bool blinkChanged;
  static uint16_t *blinkPattern;
  static size_t blinkPatternLen;
  static void ledTask(void *arg);
};

#endif