#ifndef ELECTRA_ESP_SLEEP_H
#define ELECTRA_ESP_SLEEP_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include "ha.h"

#define ESP_WAKEUP GPIO_NUM_14

class Sleep {
  public:
    static void init();
  private:
    static bool wasAwoken;
    static void gpioTask(void *arg);
    static void stopRingingTask(void *arg);
};

#endif