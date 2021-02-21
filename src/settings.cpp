#include "settings.h"

static const char *TAG = "Settings";

bool Settings::ready = false;
char *Settings::ssid = NULL;
char *Settings::pass = NULL;
char *Settings::mqttUrl = NULL;
char *Settings::mqttUser = NULL;
char *Settings::mqttPass = NULL;
char *Settings::entity = NULL;

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

char *Settings::getMqttUrl()
{
  return mqttUrl;
}

char *Settings::getMqttUser()
{
  return mqttUser;
}

char *Settings::getMqttPass()
{
  return mqttPass;
}

char *Settings::getEntity()
{
  return entity;
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
    ESP_LOGV(TAG, "Reading settings file");
    cJSON *root = cJSON_Parse((char *)buffer);
    // get wifi ssid
    if (getConfigVar(root, "ssid", ssid) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'ssid'");
      return ESP_FAIL;
    }
    // get wifi password
    if (getConfigVar(root, "pass", pass) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'pass'");
      return ESP_FAIL;
    }
    // get ha mqtt url
    if (getConfigVar(root, "mqttUrl", mqttUrl) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'mqttUrl'");
      return ESP_FAIL;
    }
    // get ha mqtt username
    if (getConfigVar(root, "mqttUser", mqttUser) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'mqttUser'");
      return ESP_FAIL;
    }
    // get ha mqtt password
    if (getConfigVar(root, "mqttPass", mqttPass) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'mqttPass'");
      return ESP_FAIL;
    }
    // get ha entity
    if (getConfigVar(root, "entity", entity) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'entity'");
      return ESP_FAIL;
    }

    ESP_LOGV(TAG, "Read settings file completed");

#if CONFIG_LOG_DEFAULT_LEVEL >= ESP_LOG_DEBUG
    ESP_LOGD(TAG, "Read settings:\nssid: [%s]\npass: [%s]\nmqtt URL: [%s]\nmqtt User: [%s]\nmqtt Password: [%s]",
             ssid, pass, mqttUrl, mqttUser, mqttPass);
#endif

    return ESP_OK;
  }
  else
  {
    ESP_LOGE(TAG, "Config file is empty");
    return ESP_FAIL;
  }
}

esp_err_t Settings::getConfigVar(cJSON *root, const char *name, char *&var)
{
  if (cJSON_HasObjectItem(root, name))
  {
    cJSON *objJSON = cJSON_GetObjectItem(root, name);
    size_t objLen = strlen(objJSON->valuestring);
    var = new char[objLen + 1];
    memcpy(var, objJSON->valuestring, objLen);
    var[objLen] = '\0';
    return ESP_OK;
  }
  else
  {
    return ESP_FAIL;
  }
}
