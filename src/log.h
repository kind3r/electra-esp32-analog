#ifndef ELECTRA_ESP_LOG_H
#define ELECTRA_ESP_LOG_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <vector>
#include <algorithm>

#define ELECTRA_ESP_LOG_INDEX "logIndex"
#define ELECTRA_ESP_LOG_SYNC 5
#define ELECTRA_ESP_LOG_MAX_SIZE 100 * 1024
#define ELECTRA_ESP_LOG_MAX_FILES 5

// typedef char *logFile_t;
// typedef logFile_t logFiles_t[ELECTRA_ESP_LOG_MAX_FILES];
typedef std::vector<std::string> logFiles_t;

class LOG
{
public:
  static esp_err_t init();
  static esp_err_t open();
  static void flush();
  static void close();
  static uint8_t list(logFiles_t &files);
  static void cleanup();
private:
  static bool ready;
  static FILE *logFile;
  static std::string logFileName;
  static uint32_t logs;
  static uint16_t getIndex();
  static void setIndex(uint16_t index);
  static int log(const char *fmt, va_list args);
};

#endif