#ifndef ELECTRA_ESP_LED_H
#define ELECTRA_ESP_LED_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/gpio.h>
#include <esp_log.h>

#define ELECTRA_ESP_LED GPIO_NUM_2

typedef struct {
  uint8_t cycles;
  uint16_t pattern[6];
} blinkPattern_t;

#define PATTERN_BLINK_OFF() {         \
  .cycles = 2,                        \
  .pattern = {0, 100, 0, 0, 0, 0}     \
}

#define PATTERN_BLINK_SLOW() {        \
  .cycles = 2,                        \
  .pattern = {1000, 1000, 0, 0, 0, 0} \
}

#define PATTERN_BLINK_FAST() {        \
  .cycles = 2,                        \
  .pattern = { 300,  300, 0, 0, 0, 0} \
}

class Led
{
public:
  static void init();
  static void setBlinkPattern(blinkPattern_t *patern);
private:
  static SemaphoreHandle_t blinkSemaphore;
  static bool blinkChanged;
  static blinkPattern_t *blinkPattern;
  static void ledTask(void *arg);
};

#endif