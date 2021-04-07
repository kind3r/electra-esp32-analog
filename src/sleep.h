#ifndef ELECTRA_ESP_SLEEP_H
#define ELECTRA_ESP_SLEEP_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <esp_wifi.h>

#include "ha.h"
#include "led.h"
#include "intercom.h"
#include "ota.h"

#define ESP_WAKEUP_INTERVAL 3 * 60 * 60 // every 3h to report battery
#define ESP_SLEEP_DELAY 1

class Sleep {
  public:
    static void init();
    static void start(int delay = ESP_SLEEP_DELAY);
  private:
    static bool started;
    static bool wasRinging;
    static int sleepDelay;
    static void sleepTask(void *arg);
    static void stopRingingTask(void *arg);
};

#endif