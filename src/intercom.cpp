#include "intercom.h"

static const char *TAG = "Intercom";

bool Intercom::swTalk = false;
bool Intercom::swOpen = false;
xQueueHandle Intercom::gpio_evt_queue = NULL;

void Intercom::init()
{
  // setup input gpios
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_POSEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = ((1ULL << ESP_SW_TALK) | (1ULL << ESP_SW_OPEN));
  io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  // setup output gpios
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = ((1ULL << ESP_TALK) | (1ULL << ESP_OPEN));
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  // run task to monitor inputs
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
  xTaskCreate(gpioTask, "gpio_task", 2048, NULL, 10, NULL);

  //install gpio isr service
  gpio_install_isr_service(0);
  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(ESP_SW_TALK, gpioHandler, (void *)ESP_SW_TALK);
  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(ESP_SW_OPEN, gpioHandler, (void *)ESP_SW_OPEN);

  pushOpen(false);
  pushTalk(false);
}

void Intercom::open() {
  pushOpen(false);
  pushTalk(false);
  vTaskDelay(200 / portTICK_RATE_MS);
  pushTalk(true);
  vTaskDelay(1000 / portTICK_RATE_MS);
  pushTalk(false);
  vTaskDelay(200 / portTICK_RATE_MS);
  pushOpen(true);
}

void IRAM_ATTR Intercom::gpioHandler(void *arg)
{
  uint32_t io_num = (uint32_t)arg;
  xQueueSendFromISR(gpio_evt_queue, &io_num, NULL);
}

void Intercom::gpioTask(void *arg)
{
  gpio_num_t io_num;
  int io_lvl;
  for (;;)
  {
    if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
    {
      io_lvl = gpio_get_level(io_num);
      ESP_LOGV(TAG, "GPIO[%d] intr, val: %d\n", io_num, io_lvl);
      if (io_num == ESP_SW_TALK)
      {
        if (io_lvl == 1 && !swTalk)
        {
          swTalk = true;
          pushTalk(swTalk);
        }
        else if (io_lvl == 0 && swTalk)
        {
          swTalk = false;
          pushTalk(swTalk);
        }
      }
      else if (io_num == ESP_SW_OPEN)
      {
        if (io_lvl == 1 && !swOpen)
        {
          swOpen = true;
          pushOpen(swOpen);
        }
        else if (io_lvl == 0 && swOpen)
        {
          swOpen = false;
          pushOpen(swOpen);
        }
      }
    }
  }
}

void Intercom::pushTalk(bool push) {
  gpio_set_level(ESP_TALK, push ? 1 : 0);
  ESP_LOGI(TAG, "Talk is %s", push ? "ON" : "OFF");
}

void Intercom::pushOpen(bool push) {
  gpio_set_level(ESP_OPEN, push ? 1 : 0);
  ESP_LOGI(TAG, "Open is %s", push ? "ON" : "OFF");
}