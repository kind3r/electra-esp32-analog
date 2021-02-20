#include "settings.h"

static const char *TAG = "Settings";

bool Settings::ready = false;
char *Settings::ssid = NULL;
char *Settings::pass = NULL;
char *Settings::ha = NULL;

esp_err_t Settings::init()
{
  if (!ready)
  {
    // init SPIFFS
    esp_err_t ret = initSPIFFS();
    if (ret != ESP_OK)
    {
      return ret;
    }
    // read and parse config file
    uint8_t configBuffer[ELECTRA_ESP_CONFIG_BUFFER_SIZE] = {0};
    ret = readConfig(configBuffer, ELECTRA_ESP_CONFIG_BUFFER_SIZE);
    if (ret != ESP_OK)
    {
      return ret;
    }

    ready = true;
  }

  return ready ? ESP_OK : ESP_FAIL;
}

char *Settings::getSsid()
{
  return ssid;
}

char *Settings::getPass()
{
  return pass;
}

char *Settings::getHa()
{
  return ha;
}

esp_err_t Settings::initSPIFFS()
{
  ESP_LOGV(TAG, "Mounting SPIFFS");

  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5, // This decides the maximum number of files that can be created on the storage
      .format_if_mount_failed = true};

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS [%s]", esp_err_to_name(ret));
    }
    return ret;
  }

  ESP_LOGV(TAG, "SPIFFS mounted");

#if CONFIG_LOG_DEFAULT_LEVEL >= ESP_LOG_DEBUG
  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information [%s]", esp_err_to_name(ret));
    return false;
  }

  ESP_LOGD(TAG, "Partition size: total: %d, used: %d", total, used);
#endif

  return ESP_OK;
}

esp_err_t Settings::readConfig(uint8_t *buffer, size_t bufferLen)
{
  ESP_LOGV(TAG, "Opening settings file");

  FILE *fd = nullptr;
  fd = fopen(ELECTRA_ESP_SETTINGS_FILE, "r");
  if (!fd)
  {
    ESP_LOGE(TAG, "Failed to read [%s] from SPIFFS", ELECTRA_ESP_SETTINGS_FILE);
    return ESP_FAIL;
  }

  size_t fileLen = fread(buffer, 1, bufferLen, fd);
  fclose(fd);

  if (fileLen > 0)
  {
    size_t objLen = 0;
    cJSON *objJSON = nullptr;
    cJSON *root = cJSON_Parse((char *)buffer);
    // get wifi ssid
    if (cJSON_HasObjectItem(root, "ssid"))
    {
      objJSON = cJSON_GetObjectItem(root, "ssid");
      objLen = strlen(objJSON->valuestring);
      ssid = new char[objLen + 1];
      memcpy(ssid, objJSON->valuestring, objLen);
      ssid[objLen] = '\0';
    }
    else
    {
      ESP_LOGE(TAG, "Config file missing 'ssid'");
      return ESP_FAIL;
    }
    // get wifi password
    if (cJSON_HasObjectItem(root, "pass"))
    {
      objJSON = cJSON_GetObjectItem(root, "pass");
      objLen = strlen(objJSON->valuestring);
      pass = new char[objLen + 1];
      memcpy(pass, objJSON->valuestring, objLen);
      pass[objLen] = '\0';
    }
    else
    {
      ESP_LOGE(TAG, "Config file missing 'pass'");
      return ESP_FAIL;
    }
    // get ha websocket url
    if (cJSON_HasObjectItem(root, "ha"))
    {
      objJSON = cJSON_GetObjectItem(root, "ha");
      objLen = strlen(objJSON->valuestring);
      ha = new char[objLen + 1];
      memcpy(ha, objJSON->valuestring, objLen);
      ha[objLen] = '\0';
    }
    else
    {
      ESP_LOGE(TAG, "Config file missing 'ha'");
      return ESP_FAIL;
    }

#if CONFIG_LOG_DEFAULT_LEVEL >= ESP_LOG_DEBUG
    ESP_LOGD(TAG, "Read settings:\nssid: [%s]\npass: [%s]\nha: [%s]", ssid, pass, ha);
#endif

    return ESP_OK;
  }
  else
  {
    ESP_LOGE(TAG, "Config file is empty");
    return ESP_FAIL;
  }
}
