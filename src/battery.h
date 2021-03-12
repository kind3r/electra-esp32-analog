#ifndef ELECTRA_ESP_BATTERY_H
#define ELECTRA_ESP_BATTERY_H

#include <stdlib.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define ESP_BAT_EN GPIO_NUM_25
#define ESP_BAT ADC1_CHANNEL_5
#define ESP_BAT_SAMPLES 32
// Resistor values for voltage divider in kOhm
#define ESP_R1 100
#define ESP_R2 10
// Battery voltage max and min
#define ESP_BAT_MAX 3200
#define ESP_BAT_MIN 2900

class Battery {
  public:
    static void init();
    static int getBatteryVoltage();
    static int getBatteryPercent();
  private:
    static int batteryVoltage;
    static int batteryPercent;
};

#endif
