#include "log.h"

static const char *TAG = "LOG";

bool LOG::ready = false;
FILE *LOG::logFile = NULL;
std::string LOG::logFileName;
uint32_t LOG::logs = 0;

esp_err_t LOG::init()
{
  if (ready)
  {
    return ESP_OK;
  }
  // get last log file number from NVS
  uint16_t index = getIndex();
  if (index == 0)
  {
    // NVS failed
    return ESP_FAIL;
  }
  logFileName = "/spiffs/log_";
  logFileName += std::to_string(index);
  logFileName += ".txt";

  struct stat st;
  int res = stat(logFileName.c_str(), &st);
  // check if log file exists
  if (res == 0)
  {
    // if file size > 100k rotate
    if (st.st_size > ELECTRA_ESP_LOG_MAX_SIZE)
    {
      index++;
      if (index < 10000)
      {
        index = 10000;
      }
      setIndex(index);
      logFileName = "/spiffs/log_";
      logFileName += std::to_string(index);
      logFileName += ".txt";

      // remove old files;
      cleanup();
    }
  }

  ESP_LOGI(TAG, "File name: %s", logFileName.c_str());
  if (open() != ESP_OK)
  {
    return ESP_FAIL;
  }

  // replace esp log handler
  esp_log_set_vprintf(&LOG::log);

  return ESP_OK;
}

esp_err_t LOG::open()
{
  if (logFile == NULL)
  {
    logFile = fopen(logFileName.c_str(), "at");
    if (logFile == NULL)
    {
      ESP_LOGE(TAG, "Cannot open logfile");
      return ESP_FAIL;
    }
  }
  return ESP_OK;
}

void LOG::flush()
{
  if (logFile != NULL)
  {
    fsync(fileno(logFile));
  }
}

void LOG::close()
{
  if (logFile != NULL)
  {
    flush();
    fclose(logFile);
    logFile = NULL;
    vTaskDelay(100 / portTICK_RATE_MS);
  }
}

uint8_t LOG::list(logFiles_t &files)
{
  ESP_LOGI(TAG, "Start log file list");
  flush();
  DIR *dir = opendir("/spiffs");
  struct dirent *entry;
  uint8_t found = 0;
  files.clear();
  while ((entry = readdir(dir)) != NULL)
  {
    // ESP_LOGI(TAG, "File found: %s", entry->d_name);
    if (entry->d_type != DT_DIR && strncmp(entry->d_name, "log_", 4) == 0)
    {
      std::string fileName(entry->d_name);
      files.push_back(fileName);
      ESP_LOGI(TAG, "File added[%d]: %s", found, files[found].c_str());
      found++;
    }
  }
  closedir(dir);
  // ESP_LOGI(TAG, "Found %d files", found);
  std::sort(files.begin(), files.end());
  return found;
}

void LOG::cleanup()
{
  // remove old files;
  logFiles_t files;
  uint8_t found = list(files);
  ESP_LOGI(TAG, "Number of existing log files: %d", found);
  if (found > ELECTRA_ESP_LOG_MAX_FILES)
  {
    std::sort(files.begin(), files.end());
    for (uint8_t i = 0; i < (found - ELECTRA_ESP_LOG_MAX_FILES); i++)
    {
      std::string fileName = "/spiffs/";
      fileName += files[i];
      ESP_LOGI(TAG, "Removing old log file: %s", fileName.c_str());
      unlink(fileName.c_str());
    }
  }
}

uint16_t LOG::getIndex()
{
  // max val  65535
  // start at 10000

  esp_err_t ret;
  nvs_handle_t handle;

  // init NVS
  ret = nvs_open("ELECTRALOG", NVS_READWRITE, &handle);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "nvs_open failed");
    return 0;
  }

  uint16_t index;
  ret = nvs_get_u16(handle, ELECTRA_ESP_LOG_INDEX, &index);
  if (ret != ESP_OK)
  {
    index = 10000;
    ret = nvs_set_u16(handle, ELECTRA_ESP_LOG_INDEX, index);
  }

  nvs_close(handle);
  return index;
}

void LOG::setIndex(uint16_t index)
{
  esp_err_t ret;
  nvs_handle_t handle;

  // init NVS
  ret = nvs_open("ELECTRALOG", NVS_READWRITE, &handle);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "nvs_open failed");
    return;
  }

  ret = nvs_set_u16(handle, ELECTRA_ESP_LOG_INDEX, index);
  nvs_close(handle);
}

int LOG::log(const char *fmt, va_list args)
{
  if (logFile != NULL)
  {
    vfprintf(logFile, fmt, args);
    logs++;
    if (logs % ELECTRA_ESP_LOG_SYNC == 0)
    {
      flush();
    }
  }

  return vprintf(fmt, args);
}
