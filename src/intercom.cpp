#include "intercom.h"

static const char *TAG = "Intercom";

bool Intercom::swTalk = false;
bool Intercom::swOpen = false;

void Intercom::init()
{
  // setup output gpios
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = ((1ULL << ELECTRA_ESP_TALK) | (1ULL << ELECTRA_ESP_OPEN));
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  // setup input gpios
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << ELECTRA_ESP_RINGING);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  pushOpen(false);
  pushTalk(false);
}

bool Intercom::isRinging() {
  return (gpio_get_level(ELECTRA_ESP_RINGING) == 1);
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
  vTaskDelay(2000 / portTICK_RATE_MS);
  pushOpen(false);
}

void Intercom::pushTalk(bool push) {
  gpio_set_level(ELECTRA_ESP_TALK, push ? 1 : 0);
  ESP_LOGI(TAG, "Talk is %s", push ? "ON" : "OFF");
}

void Intercom::pushOpen(bool push) {
  gpio_set_level(ELECTRA_ESP_OPEN, push ? 1 : 0);
  ESP_LOGI(TAG, "Open is %s", push ? "ON" : "OFF");
}