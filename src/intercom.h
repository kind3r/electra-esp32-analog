#ifndef ELECTRA_ESP_INTERCOM_H
#define ELECTRA_ESP_INTERCOM_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_log.h>

#define ESP_SW_TALK GPIO_NUM_14
#define ESP_SW_OPEN GPIO_NUM_15
#define ESP_TALK GPIO_NUM_12
#define ESP_OPEN GPIO_NUM_13

class Intercom
{
public:
  static void init();
  static void open();
private:
  static bool swTalk;
  static bool swOpen;
  static xQueueHandle gpio_evt_queue;
  static void IRAM_ATTR gpioHandler(void* arg);
  static void gpioTask(void *arg);
  static void pushTalk(bool push);
  static void pushOpen(bool push);
};

#endif