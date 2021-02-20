#include "util.h"

static const char *TAG = "util";

void meminfo()
{
  const int min_free_8bit_cap = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  const int min_free_32bit_cap = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT);

  ESP_LOGI(TAG, "||   Miniumum Free DRAM\t|   Minimum Free IRAM\t||");
  ESP_LOGI(TAG, "||\t%-6d\t\t|\t%-6d\t\t||",
         min_free_8bit_cap, (min_free_32bit_cap - min_free_8bit_cap));
}

void meminfo(const char description[]) {
  ESP_LOGI(TAG, "%s", description);
  meminfo();
}