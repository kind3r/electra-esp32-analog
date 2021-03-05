#include "battery.h"

static const char *TAG = "Battery";

int Battery::batteryVoltage = -1;
int Battery::batteryPercent = -1;

void Battery::init()
{
  // setup enable pin
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << ESP_BAT_EN);
  io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
  gpio_set_level(ESP_BAT_EN, 1);

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ESP_BAT, ADC_ATTEN_DB_0);

  //Characterize ADC at particular atten
  esp_adc_cal_characteristics_t *adc_chars = new esp_adc_cal_characteristics_t;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 1100, adc_chars);
  //Check type of calibration value used to characterize ADC
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
  {
    ESP_LOGI(TAG, "eFuse Vref");
  }
  else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
  {
    ESP_LOGI(TAG, "Two Point");
  }
  else
  {
    ESP_LOGI(TAG, "Default");
  }

  //Multisampling
  uint32_t adc_reading = 0;
  for (int i = 0; i < ESP_BAT_SAMPLES; i++)
  {
    adc_reading += adc1_get_raw(ESP_BAT);
  }
  adc_reading /= ESP_BAT_SAMPLES;
  ESP_LOGI(TAG, "ADC reading: %d", adc_reading);
  uint32_t adcVoltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
  ESP_LOGI(TAG, "ADC Voltage: %d", adcVoltage);

  // 0mV - 1100mV => 0mV - 3600mV based on voltage divider resistors
  // voltage divider reverse computation
  batteryVoltage = adcVoltage * (ESP_R1 + ESP_R2) / ESP_R2;
  ESP_LOGI(TAG, "Voltage: %d", batteryVoltage);
  if (batteryVoltage >= ESP_BAT_MAX)
  {
    batteryPercent = 100;
  }
  else if (batteryVoltage <= ESP_BAT_MIN)
  {
    batteryPercent = 0;
  }
  else
  {
    batteryPercent = (batteryVoltage - ESP_BAT_MIN) * 100 / (ESP_BAT_MAX - ESP_BAT_MIN);
  }
  ESP_LOGI(TAG, "Percent: %d", batteryPercent);

  gpio_set_level(ESP_BAT_EN, 0);

  delete adc_chars;
}

int Battery::getBatteryVoltage()
{
  return batteryVoltage;
}

int Battery::getBatteryPercent()
{
  return batteryPercent;
}